/*
 *  Copyright 2019-2024 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "Tutorial22_HybridRendering.hpp"

#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ShaderMacroHelper.hpp"
#include "imgui.h"
#include "ImGuiUtils.hpp"
#include "../imGuIZMO.quat/imGuIZMO.h"
#include "Align.hpp"

namespace Diligent
{

static_assert(sizeof(HLSL::GlobalConstants) % 16 == 0, "Structure must be 16-byte aligned");
static_assert(sizeof(HLSL::ObjectConstants) % 16 == 0, "Structure must be 16-byte aligned");

SampleBase* CreateSample()
{
    return new Tutorial22_HybridRendering();
}

struct AABB
{
    float3 min;
    float3 max;
};

std::vector<AABB> MazeWalls;

struct Key
{
    float3           Min;
    float3           Max;
    bool             Collected = false;
    int              ObjectIdx = -1; 
    int              WallIdx   = -1; 
    std::vector<int> DoorIds;
};

std::vector<Key> m_Keys;
int              m_KeysCollected = 0;

struct Door
{
    int      WallIdx;
    int      ObjectIdx;
    bool     Opened    = false;
    bool     Rising    = false;
    float    RiseTimer = 0.0f; 
    float    RiseSpeed = 2.0f; 
    float4x4 OriginalMat;      
    int      Id;              
    int      BlockType;
};
std::vector<Door> m_Doors;

bool  m_ShowUnlockMsg  = false;
float m_UnlockMsgTimer = 0.0f;
float m_UnlockMsgTime  = 3.0f; 

struct KeyDoorBinding
{
    int KeyBlockType;
    int DoorBlockType;
};

std::vector<KeyDoorBinding> m_KeyDoorBindings;


void Tutorial22_HybridRendering::CreateSceneMaterials(uint2& CubeMaterialRange, Uint32& GroundMaterial, std::vector<HLSL::MaterialAttribs>& Materials)
{
    Uint32 AnisotropicClampSampInd = 0;
    Uint32 AnisotropicWrapSampInd  = 0;

    // Create samplers
    {
        const SamplerDesc AnisotropicClampSampler{
            FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC,
            TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, 0.f, 8 //
        };
        const SamplerDesc AnisotropicWrapSampler{
            FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC,
            TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, 0.f, 8 //
        };

        RefCntAutoPtr<ISampler> pSampler;
        m_pDevice->CreateSampler(AnisotropicClampSampler, &pSampler);
        AnisotropicClampSampInd = static_cast<Uint32>(m_Scene.Samplers.size());
        m_Scene.Samplers.push_back(std::move(pSampler));

        pSampler = nullptr;
        m_pDevice->CreateSampler(AnisotropicWrapSampler, &pSampler);
        AnisotropicWrapSampInd = static_cast<Uint32>(m_Scene.Samplers.size());
        m_Scene.Samplers.push_back(std::move(pSampler));
    }

    const auto LoadMaterial = [&](const char* ColorMapName, const float4& BaseColor, Uint32 SamplerInd) //
    {
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB       = true;
        loadInfo.GenerateMips = true;
        RefCntAutoPtr<ITexture> Tex;
        CreateTextureFromFile(ColorMapName, loadInfo, m_pDevice, &Tex);
        VERIFY_EXPR(Tex);

        HLSL::MaterialAttribs mtr;
        mtr.SampInd         = SamplerInd;
        mtr.BaseColorMask   = BaseColor;
        mtr.BaseColorTexInd = static_cast<Uint32>(m_Scene.Textures.size());
        m_Scene.Textures.push_back(std::move(Tex));
        Materials.push_back(mtr);
    };

    // Cube materials
    CubeMaterialRange.x = static_cast<Uint32>(Materials.size());
    LoadMaterial("DGLogo0.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo1.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("payaso.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("bichoraro.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("ExitHell.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("ExitHell1.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("ExitHell2.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("Techo.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo4.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("DGLogo5.jpeg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("payaso2.png", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);
    LoadMaterial("key.jpg", float4{1.f}, AnisotropicClampSampInd);

    CubeMaterialRange.y = static_cast<Uint32>(Materials.size());

    // Ground material
    GroundMaterial = static_cast<Uint32>(Materials.size());
    LoadMaterial("Marble.jpg", float4{1.f}, AnisotropicWrapSampInd);
}

Tutorial22_HybridRendering::Mesh Tutorial22_HybridRendering::CreateTexturedPlaneMesh(IRenderDevice* pDevice, float2 UVScale)
{
    Mesh PlaneMesh;
    PlaneMesh.Name = "Ground";

    {
        struct PlaneVertex // Alias for HLSL::Vertex
        {
            float3 pos;
            float3 norm;
            float2 uv;
        };
        static_assert(sizeof(PlaneVertex) == sizeof(HLSL::Vertex), "Vertex size mismatch");

        // clang-format off
        const PlaneVertex Vertices[] = 
        {
            {float3{-1, 0, -1}, float3{0, 1, 0}, float2{0,         0        }},
            {float3{ 1, 0, -1}, float3{0, 1, 0}, float2{UVScale.x, 0        }},
            {float3{-1, 0,  1}, float3{0, 1, 0}, float2{0,         UVScale.y}},
            {float3{ 1, 0,  1}, float3{0, 1, 0}, float2{UVScale.x, UVScale.y}}
        };
        // clang-format on
        PlaneMesh.NumVertices = _countof(Vertices);

        BufferDesc VBDesc;
        VBDesc.Name              = "Plane vertex buffer";
        VBDesc.Usage             = USAGE_IMMUTABLE;
        VBDesc.BindFlags         = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE | BIND_RAY_TRACING;
        VBDesc.Size              = sizeof(Vertices);
        VBDesc.Mode              = BUFFER_MODE_STRUCTURED;
        VBDesc.ElementByteStride = sizeof(Vertices[0]);
        BufferData VBData{Vertices, VBDesc.Size};
        pDevice->CreateBuffer(VBDesc, &VBData, &PlaneMesh.VertexBuffer);
    }

    {
        const Uint32 Indices[] = {0, 2, 3, 3, 1, 0};
        PlaneMesh.NumIndices   = _countof(Indices);

        BufferDesc IBDesc;
        IBDesc.Name              = "Plane index buffer";
        IBDesc.BindFlags         = BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE | BIND_RAY_TRACING;
        IBDesc.Size              = sizeof(Indices);
        IBDesc.Mode              = BUFFER_MODE_STRUCTURED;
        IBDesc.ElementByteStride = sizeof(Indices[0]);
        BufferData IBData{Indices, IBDesc.Size};
        pDevice->CreateBuffer(IBDesc, &IBData, &PlaneMesh.IndexBuffer);
    }

    return PlaneMesh;
}

void Tutorial22_HybridRendering::CreateSceneObjects(const uint2 CubeMaterialRange, const Uint32 GroundMaterial)
{
    Uint32 CubeMeshId  = 0;
    Uint32 PlaneMeshId = 0;

    // Create meshes
    {
        Mesh CubeMesh;
        CubeMesh.Name = "Cube";
        GeometryPrimitiveBuffersCreateInfo CubeBuffersCI;
        CubeBuffersCI.VertexBufferBindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE | BIND_RAY_TRACING;
        CubeBuffersCI.IndexBufferBindFlags  = BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE | BIND_RAY_TRACING;
        CubeBuffersCI.VertexBufferMode      = BUFFER_MODE_STRUCTURED;
        CubeBuffersCI.IndexBufferMode       = BUFFER_MODE_STRUCTURED;
        GeometryPrimitiveInfo CubeGeoInfo;
        CreateGeometryPrimitiveBuffers(m_pDevice, CubeGeometryPrimitiveAttributes{2.f, GEOMETRY_PRIMITIVE_VERTEX_FLAG_ALL},
                                       &CubeBuffersCI, &CubeMesh.VertexBuffer, &CubeMesh.IndexBuffer, &CubeGeoInfo);
        CubeMesh.NumVertices = CubeGeoInfo.NumVertices;
        CubeMesh.NumIndices  = CubeGeoInfo.NumIndices;

        auto PlaneMesh = CreateTexturedPlaneMesh(m_pDevice, float2{25});

        const auto RTProps = m_pDevice->GetAdapterInfo().RayTracing;

        // Cube mesh will be copied to the beginning of the buffers
        CubeMesh.FirstVertex = 0;
        CubeMesh.FirstIndex  = 0;
        // Plane mesh data will reside after the cube. Offsets must be properly aligned!
        PlaneMesh.FirstVertex = AlignUp(CubeMesh.NumVertices * Uint32{sizeof(HLSL::Vertex)}, RTProps.VertexBufferAlignment) / sizeof(HLSL::Vertex);
        PlaneMesh.FirstIndex  = AlignUp(CubeMesh.NumIndices * Uint32{sizeof(uint)}, RTProps.IndexBufferAlignment) / sizeof(uint);

        // Merge vertex buffers
        {
            BufferDesc VBDesc;
            VBDesc.Name              = "Shared vertex buffer";
            VBDesc.BindFlags         = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE | BIND_RAY_TRACING;
            VBDesc.Size              = (Uint64{PlaneMesh.FirstVertex} + Uint64{PlaneMesh.NumVertices}) * sizeof(HLSL::Vertex);
            VBDesc.Mode              = BUFFER_MODE_STRUCTURED;
            VBDesc.ElementByteStride = sizeof(HLSL::Vertex);

            RefCntAutoPtr<IBuffer> pSharedVB;
            m_pDevice->CreateBuffer(VBDesc, nullptr, &pSharedVB);

            // Copy cube vertices
            m_pImmediateContext->CopyBuffer(CubeMesh.VertexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                            pSharedVB, CubeMesh.FirstVertex * sizeof(HLSL::Vertex), CubeMesh.NumVertices * sizeof(HLSL::Vertex),
                                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            // Copy plane vertices
            m_pImmediateContext->CopyBuffer(PlaneMesh.VertexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                            pSharedVB, PlaneMesh.FirstVertex * sizeof(HLSL::Vertex), PlaneMesh.NumVertices * sizeof(HLSL::Vertex),
                                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            CubeMesh.VertexBuffer  = pSharedVB;
            PlaneMesh.VertexBuffer = pSharedVB;
        }

        // Merge index buffers
        {
            BufferDesc IBDesc;
            IBDesc.Name              = "Shared index buffer";
            IBDesc.BindFlags         = BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE | BIND_RAY_TRACING;
            IBDesc.Size              = (Uint64{PlaneMesh.FirstIndex} + Uint64{PlaneMesh.NumIndices}) * sizeof(uint);
            IBDesc.Mode              = BUFFER_MODE_STRUCTURED;
            IBDesc.ElementByteStride = sizeof(uint);

            RefCntAutoPtr<IBuffer> pSharedIB;
            m_pDevice->CreateBuffer(IBDesc, nullptr, &pSharedIB);

            // Copy cube indices
            m_pImmediateContext->CopyBuffer(CubeMesh.IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                            pSharedIB, CubeMesh.FirstIndex * sizeof(uint), CubeMesh.NumIndices * sizeof(uint),
                                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            // Copy plane indices
            m_pImmediateContext->CopyBuffer(PlaneMesh.IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                            pSharedIB, PlaneMesh.FirstIndex * sizeof(uint), PlaneMesh.NumIndices * sizeof(uint),
                                            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            CubeMesh.IndexBuffer  = pSharedIB;
            PlaneMesh.IndexBuffer = pSharedIB;
        }

        CubeMeshId = static_cast<Uint32>(m_Scene.Meshes.size());
        m_Scene.Meshes.push_back(CubeMesh);
        PlaneMeshId = static_cast<Uint32>(m_Scene.Meshes.size());
        m_Scene.Meshes.push_back(PlaneMesh);
    }
    const int mazeRows = 50;
    const int mazeCols = 100;

    int maze[mazeRows][mazeCols] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 19, 1, 1, 1, 1, 1, 1, 1, 1, 1, 19, 1, 1, 1, 1},
        {1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19},
        {1, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 3, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 3, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 1, 1, 0, 24, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19},
        {1, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 3, 3, 3, 3, 3, 14, 14, 14, 14, 14, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 15, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 12, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 13, 13, 13, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 25, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 5, 5, 5, 1, 13, 13, 13, 1, 5, 5, 5, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 6, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 3, 1, 3, 1, 3, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 27, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 4, 0, 0, 0, 0, 0, 21, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1},
        {1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 4, 1, 2, 1, 10, 10, 10, 10, 1, 2, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 17, 17, 17, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 20, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

    std::vector<std::vector<bool>> visited(mazeRows, std::vector<bool>(mazeCols, false));
    m_KeyDoorBindings.push_back({20, 10});
    m_KeyDoorBindings.push_back({21, 11});
    m_KeyDoorBindings.push_back({22, 12});
    m_KeyDoorBindings.push_back({23, 13});
    m_KeyDoorBindings.push_back({24, 14});
    m_KeyDoorBindings.push_back({25, 15});
    m_KeyDoorBindings.push_back({26, 16});
    m_KeyDoorBindings.push_back({27, 17});

    // PASADA 1: Muros, puertas y bloques especiales
    for (int z = 0; z < mazeRows; ++z)
    {
        for (int x = 0; x < mazeCols; ++x)
        {
            if (!visited[z][x] && maze[z][x] != 0)
            {
                int blockType = maze[z][x];

                if (blockType == 1)
                {
                    visited[z][x] = true;
                    float spacing = 2.0f;
                    float scaleY  = 3.0f;
                    float posY    = scaleY - 0.2f;
                    float scaleX  = spacing * 0.5f;
                    float scaleZ  = spacing * 0.5f;
                    float posX    = (x - mazeCols / 2.0f) * spacing;
                    float posZ    = (z - mazeRows / 2.0f) * spacing;

                    int materialOffset = CubeMaterialRange.x + (blockType - 1);

                    HLSL::ObjectAttribs obj;
                    obj.ModelMat = (float4x4::Scale(scaleX, scaleY, scaleZ) *
                                    float4x4::Translation(posX, posY, posZ))
                                       .Transpose();
                    obj.NormalMat   = obj.ModelMat;
                    obj.MaterialId  = materialOffset;
                    obj.MeshId      = CubeMeshId;
                    obj.FirstIndex  = m_Scene.Meshes[obj.MeshId].FirstIndex;
                    obj.FirstVertex = m_Scene.Meshes[obj.MeshId].FirstVertex;
                    m_Scene.Objects.push_back(obj);

                    float3 wallMin = {posX - scaleX, 0.0f, posZ - scaleZ};
                    float3 wallMax = {posX + scaleX, scaleY, posZ + scaleZ};
                    MazeWalls.push_back({wallMin, wallMax});
                }
                else if (blockType >= 10 && blockType <= 17) // puerta
                {
                    int runX = 1;
                    while (x + runX < mazeCols && maze[z][x + runX] == blockType && !visited[z][x + runX])
                        runX++;
                    int runZ = 1;
                    while (z + runZ < mazeRows && maze[z + runZ][x] == blockType && !visited[z + runZ][x])
                        runZ++;

                    bool horizontal = runX >= runZ;
                    int  runLength  = horizontal ? runX : runZ;

                    for (int i = 0; i < runLength; ++i)
                    {
                        if (horizontal)
                            visited[z][x + i] = true;
                        else
                            visited[z + i][x] = true;
                    }

                    float spacing = 2.0f;
                    float scaleY  = 3.0f;
                    float posY    = scaleY - 0.2f;
                    float posX, posZ, scaleX, scaleZ;

                    if (horizontal)
                    {
                        scaleX = spacing * runLength * 0.5f;
                        scaleZ = spacing * 0.5f;
                        posX   = (x + (runLength - 1) * 0.5f - mazeCols / 2.0f) * spacing;
                        posZ   = (z - mazeRows / 2.0f) * spacing;
                    }
                    else
                    {
                        scaleX = spacing * 0.5f;
                        scaleZ = spacing * runLength * 0.5f;
                        posX   = (x - mazeCols / 2.0f) * spacing;
                        posZ   = (z + (runLength - 1) * 0.5f - mazeRows / 2.0f) * spacing;
                    }

                    int materialOffset = CubeMaterialRange.x + (blockType - 1);

                    HLSL::ObjectAttribs obj;
                    obj.ModelMat = (float4x4::Scale(scaleX, scaleY, scaleZ) *
                                    float4x4::Translation(posX, posY, posZ))
                                       .Transpose();
                    obj.NormalMat   = obj.ModelMat;
                    obj.MaterialId  = materialOffset;
                    obj.MeshId      = CubeMeshId;
                    obj.FirstIndex  = m_Scene.Meshes[obj.MeshId].FirstIndex;
                    obj.FirstVertex = m_Scene.Meshes[obj.MeshId].FirstVertex;

                    int objIdx = static_cast<int>(m_Scene.Objects.size());
                    m_Scene.Objects.push_back(obj);

                    float3 wallMin = {posX - scaleX, 0.0f, posZ - scaleZ};
                    float3 wallMax = {posX + scaleX, scaleY, posZ + scaleZ};
                    int    wallIdx = static_cast<int>(MazeWalls.size());
                    MazeWalls.push_back({wallMin, wallMax});

                    Door door;
                    door.WallIdx     = wallIdx;
                    door.ObjectIdx   = objIdx;
                    door.Opened      = false;
                    door.Rising      = false;
                    door.RiseTimer   = 0.0f;
                    door.OriginalMat = {};
                    door.Id          = m_nextDoorId++;
                    m_Doors.push_back(door);
                }
                else if (blockType >= 2 && blockType <= 9 || blockType == 19)
                {
                    int runX = 1;
                    while (x + runX < mazeCols && maze[z][x + runX] == blockType && !visited[z][x + runX])
                        runX++;

                    int runZ = 1;
                    while (z + runZ < mazeRows && maze[z + runZ][x] == blockType && !visited[z + runZ][x])
                        runZ++;

                    bool horizontal = runX >= runZ;
                    int  runLength  = horizontal ? runX : runZ;

                    for (int i = 0; i < runLength; ++i)
                    {
                        if (horizontal)
                            visited[z][x + i] = true;
                        else
                            visited[z + i][x] = true;
                    }

                    float spacing = 2.0f;
                    float scaleY  = 3.0f;
                    float posY    = scaleY - 0.2f;

                    float posX, posZ, scaleX, scaleZ;

                    if (horizontal)
                    {
                        scaleX = spacing * runLength * 0.5f;
                        scaleZ = spacing * 0.5f;
                        posX   = (x + (runLength - 1) / 2.0f - mazeCols / 2.0f) * spacing;
                        posZ   = (z - mazeRows / 2.0f) * spacing;
                    }
                    else
                    {
                        scaleX = spacing * 0.5f;
                        scaleZ = spacing * runLength * 0.5f;
                        posX   = (x - mazeCols / 2.0f) * spacing;
                        posZ   = (z + (runLength - 1) / 2.0f - mazeRows / 2.0f) * spacing;
                    }

                    int materialOffset = CubeMaterialRange.x + (blockType - 1);

                    HLSL::ObjectAttribs obj;
                    obj.ModelMat = (float4x4::Scale(scaleX, scaleY, scaleZ) *
                                    float4x4::Translation(posX, posY, posZ))
                                       .Transpose();
                    obj.NormalMat   = obj.ModelMat;
                    obj.MaterialId  = materialOffset;
                    obj.MeshId      = CubeMeshId;
                    obj.FirstIndex  = m_Scene.Meshes[obj.MeshId].FirstIndex;
                    obj.FirstVertex = m_Scene.Meshes[obj.MeshId].FirstVertex;
                    m_Scene.Objects.push_back(obj);

                    float3 wallMin = {posX - scaleX, 0.0f, posZ - scaleZ};
                    float3 wallMax = {posX + scaleX, scaleY, posZ + scaleZ};
                    MazeWalls.push_back({wallMin, wallMax});
                }
            }
        }
    }

    // PASADA 2: Llaves
    for (int z = 0; z < mazeRows; ++z)
    {
        for (int x = 0; x < mazeCols; ++x)
        {
            int blockType = maze[z][x];
            if (blockType >= 20 && blockType <= 27 && !visited[z][x])
            {
                visited[z][x] = true;

                float spacing = 2.0f;
                float size    = 0.5f;
                float posX    = (x - mazeCols / 2.0f) * spacing;
                float posY    = size + 2.0f;
                float posZ    = (z - mazeRows / 2.0f) * spacing;

                HLSL::ObjectAttribs keyObj;
                keyObj.ModelMat = (float4x4::Scale(size, size, size) *
                                   float4x4::Translation(posX, posY, posZ))
                                      .Transpose();
                keyObj.NormalMat   = keyObj.ModelMat;
                keyObj.MaterialId  = CubeMaterialRange.x + (blockType - 1);
                keyObj.MeshId      = CubeMeshId;
                keyObj.FirstIndex  = m_Scene.Meshes[keyObj.MeshId].FirstIndex;
                keyObj.FirstVertex = m_Scene.Meshes[keyObj.MeshId].FirstVertex;

                int objIdx = static_cast<int>(m_Scene.Objects.size());
                m_Scene.Objects.push_back(keyObj);

                Key newKey;
                newKey.Min       = {posX - size, posY - size, posZ - size};
                newKey.Max       = {posX + size, posY + size, posZ + size};
                newKey.ObjectIdx = objIdx;

                int doorType = -1;
                for (const auto& binding : m_KeyDoorBindings)
                {
                    if (binding.KeyBlockType == blockType)
                    {
                        doorType = binding.DoorBlockType;
                        break;
                    }
                }

                for (const Door& door : m_Doors)
                {
                    const HLSL::ObjectAttribs& doorObj = m_Scene.Objects[door.ObjectIdx];
                    if (doorObj.MaterialId == CubeMaterialRange.x + (doorType - 1))
                    {
                        newKey.DoorIds.push_back(door.Id);
                    }
                }


                m_Keys.push_back(newKey);
            }
        }
    }


    // Crear instancia para todos los cubos del laberinto
    InstancedObjects InstObj;
    InstObj.MeshInd             = CubeMeshId;
    InstObj.NumObjects          = static_cast<Uint32>(m_Scene.Objects.size());
    InstObj.ObjectAttribsOffset = 0;
    m_Scene.ObjectInstances.push_back(InstObj);


    // Crear plano del suelo
    InstObj.ObjectAttribsOffset = static_cast<Uint32>(m_Scene.Objects.size());
    InstObj.MeshInd             = PlaneMeshId;
    {
        HLSL::ObjectAttribs obj;
        obj.ModelMat    = (float4x4::Scale(100.f, 1.f, 50.f) * float4x4::Translation(0.f, -0.2f, 0.f)).Transpose();
        obj.NormalMat   = float3x3::Identity();
        obj.MaterialId  = GroundMaterial;
        obj.MeshId      = PlaneMeshId;
        obj.FirstIndex  = m_Scene.Meshes[obj.MeshId].FirstIndex;
        obj.FirstVertex = m_Scene.Meshes[obj.MeshId].FirstVertex;
        m_Scene.Objects.push_back(obj);
    }
    InstObj.NumObjects = static_cast<Uint32>(m_Scene.Objects.size()) - InstObj.ObjectAttribsOffset;
    m_Scene.ObjectInstances.push_back(InstObj);

    // Crear techo
    InstancedObjects ceilingInst;
    ceilingInst.MeshInd             = CubeMeshId;
    ceilingInst.ObjectAttribsOffset = static_cast<Uint32>(m_Scene.Objects.size());

    {
        HLSL::ObjectAttribs obj;

        float spacing       = 2.0f;
        float mazeWidth     = mazeCols * spacing;
        float mazeDepth     = mazeRows * spacing;
        float thickness     = 0.5f;
        float ceilingHeight = 6.0f;

        float scaleX = mazeWidth * 0.5f;
        float scaleY = thickness;
        float scaleZ = mazeDepth * 0.5f;

        float posX = 0.0f;
        float posY = ceilingHeight + scaleY * 0.5f;
        float posZ = 0.0f;

        obj.ModelMat = (float4x4::Scale(scaleX, scaleY, scaleZ) *
                        float4x4::Translation(posX, posY, posZ))
                           .Transpose();

        obj.NormalMat   = obj.ModelMat;
        obj.MaterialId  = CubeMaterialRange.x + 8;
        obj.MeshId      = CubeMeshId;
        obj.FirstIndex  = m_Scene.Meshes[obj.MeshId].FirstIndex;
        obj.FirstVertex = m_Scene.Meshes[obj.MeshId].FirstVertex;

        m_Scene.Objects.push_back(obj);
    }

    ceilingInst.NumObjects = static_cast<Uint32>(m_Scene.Objects.size()) - ceilingInst.ObjectAttribsOffset;
    m_Scene.ObjectInstances.push_back(ceilingInst);


    // Crear instancia para el monstruo
    InstancedObjects monsterInst;
    monsterInst.MeshInd             = CubeMeshId;
    monsterInst.ObjectAttribsOffset = static_cast<Uint32>(m_Scene.Objects.size());

    {
        HLSL::ObjectAttribs obj;
        float3              startPos     = float3{0.f, 3.f, -20.f};
        float               monsterScale = 0.01f;

        obj.ModelMat = (float4x4::Scale(0.01f, monsterScale, monsterScale) *
                        float4x4::Translation(startPos))
                           .Transpose();
        obj.NormalMat   = float3x3::Identity();
        obj.MaterialId  = CubeMaterialRange.x + 17;
        obj.MeshId      = CubeMeshId;
        obj.FirstIndex  = m_Scene.Meshes[obj.MeshId].FirstIndex;
        obj.FirstVertex = m_Scene.Meshes[obj.MeshId].FirstVertex;

        Uint32 monsterIndex = static_cast<Uint32>(m_Scene.Objects.size());
        m_Scene.Objects.push_back(obj);

        //  dinámico para poder moverlo
        m_Scene.DynamicObjects.push_back({monsterIndex});
    }
    monsterInst.NumObjects = 1;
    m_Scene.ObjectInstances.push_back(monsterInst);
}

void Tutorial22_HybridRendering::HandleCollisions(float3& CameraPos, float CamRadius)
{
    for (const auto& wall : MazeWalls)
    {
        float3 closestPoint;
        closestPoint.x = std::max(wall.min.x, std::min(CameraPos.x, wall.max.x));
        closestPoint.y = std::max(wall.min.y, std::min(CameraPos.y, wall.max.y));
        closestPoint.z = std::max(wall.min.z, std::min(CameraPos.z, wall.max.z));

        float3 delta    = CameraPos - closestPoint;
        float  distance = length(delta);

        if (distance < CamRadius)
        {
            float3 collisionNormal  = delta / distance;
            float  penetrationDepth = CamRadius - distance;

            CameraPos += collisionNormal * penetrationDepth * 1.1f;
        }
    }
}

void Tutorial22_HybridRendering::HandleKeyCollection(const float3& camPos, float camRadius)
{
    for (auto& key : m_Keys)
    {
        if (key.Collected) continue;

        float3 closest;
        closest.x = std::max(key.Min.x, std::min(camPos.x, key.Max.x));
        closest.y = std::max(key.Min.y, std::min(camPos.y, key.Max.y));
        closest.z = std::max(key.Min.z, std::min(camPos.z, key.Max.z));

        float3 delta = camPos - closest;
        float  dist  = length(delta);

        if (dist < camRadius)
        {
            key.Collected    = true;
            m_ShowUnlockMsg  = true;
            m_UnlockMsgTimer = 0.0f;

            for (int doorId : key.DoorIds)
            {
                for (auto& door : m_Doors)
                {
                    if (door.Id == doorId && !door.Opened)
                    {
                        door.Opened      = true;
                        door.Rising      = true;
                        door.RiseTimer   = 0.0f;
                        door.OriginalMat = m_Scene.Objects[door.ObjectIdx].ModelMat;
                        break;
                    }
                }
            }

            auto& obj    = m_Scene.Objects[key.ObjectIdx];
            obj.ModelMat = float4x4::Scale(0.0f, 0.0f, 0.0f).Transpose();
        }
    }
}


void Tutorial22_HybridRendering::TryOpenDoors()
{
    if (m_KeysCollected > 0)
    {
        for (auto& door : m_Doors)
        {
            if (!door.Opened)
            {
                door.Opened    = true;
                door.Rising    = true;
                door.RiseTimer = 0.0f;
                // Guardar la mat original:
                door.OriginalMat = m_Scene.Objects[door.ObjectIdx].ModelMat;
            }
        }
        m_KeysCollected = 0;
    }
}

void Tutorial22_HybridRendering::CreateSceneAccelStructs()
{
    // Create and build bottom-level acceleration structure
    {
        RefCntAutoPtr<IBuffer> pScratchBuffer;

        for (auto& Mesh : m_Scene.Meshes)
        {
            // Create BLAS
            BLASTriangleDesc Triangles;
            {
                Triangles.GeometryName         = Mesh.Name.c_str();
                Triangles.MaxVertexCount       = Mesh.NumVertices;
                Triangles.VertexValueType      = VT_FLOAT32;
                Triangles.VertexComponentCount = 3;
                Triangles.MaxPrimitiveCount    = Mesh.NumIndices / 3;
                Triangles.IndexType            = VT_UINT32;

                const auto BLASName{Mesh.Name + " BLAS"};

                BottomLevelASDesc ASDesc;
                ASDesc.Name          = BLASName.c_str();
                ASDesc.Flags         = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
                ASDesc.pTriangles    = &Triangles;
                ASDesc.TriangleCount = 1;
                m_pDevice->CreateBLAS(ASDesc, &Mesh.BLAS);
            }

            // Create or reuse scratch buffer; this will insert the barrier between BuildBLAS invocations, which may be suboptimal.
            if (!pScratchBuffer || pScratchBuffer->GetDesc().Size < Mesh.BLAS->GetScratchBufferSizes().Build)
            {
                BufferDesc BuffDesc;
                BuffDesc.Name      = "BLAS Scratch Buffer";
                BuffDesc.Usage     = USAGE_DEFAULT;
                BuffDesc.BindFlags = BIND_RAY_TRACING;
                BuffDesc.Size      = Mesh.BLAS->GetScratchBufferSizes().Build;

                pScratchBuffer = nullptr;
                m_pDevice->CreateBuffer(BuffDesc, nullptr, &pScratchBuffer);
            }

            // Build BLAS
            BLASBuildTriangleData TriangleData;
            TriangleData.GeometryName         = Triangles.GeometryName;
            TriangleData.pVertexBuffer        = Mesh.VertexBuffer;
            TriangleData.VertexStride         = Mesh.VertexBuffer->GetDesc().ElementByteStride;
            TriangleData.VertexOffset         = Uint64{Mesh.FirstVertex} * Uint64{TriangleData.VertexStride};
            TriangleData.VertexCount          = Mesh.NumVertices;
            TriangleData.VertexValueType      = Triangles.VertexValueType;
            TriangleData.VertexComponentCount = Triangles.VertexComponentCount;
            TriangleData.pIndexBuffer         = Mesh.IndexBuffer;
            TriangleData.IndexOffset          = Uint64{Mesh.FirstIndex} * Uint64{Mesh.IndexBuffer->GetDesc().ElementByteStride};
            TriangleData.PrimitiveCount       = Triangles.MaxPrimitiveCount;
            TriangleData.IndexType            = Triangles.IndexType;
            TriangleData.Flags                = RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            BuildBLASAttribs Attribs;
            Attribs.pBLAS             = Mesh.BLAS;
            Attribs.pTriangleData     = &TriangleData;
            Attribs.TriangleDataCount = 1;

            // Scratch buffer will be used to store temporary data during the BLAS build.
            // Previous content in the scratch buffer will be discarded.
            Attribs.pScratchBuffer = pScratchBuffer;

            // Allow engine to change resource states.
            Attribs.BLASTransitionMode          = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            Attribs.GeometryTransitionMode      = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            Attribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

            m_pImmediateContext->BuildBLAS(Attribs);
        }
    }

    // Create TLAS
    {
        TopLevelASDesc TLASDesc;
        TLASDesc.Name             = "Scene TLAS";
        TLASDesc.MaxInstanceCount = static_cast<Uint32>(m_Scene.Objects.size());
        TLASDesc.Flags            = RAYTRACING_BUILD_AS_ALLOW_UPDATE | RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
        m_pDevice->CreateTLAS(TLASDesc, &m_Scene.TLAS);
    }
}

void Tutorial22_HybridRendering::UpdateTLAS()
{
    const Uint32 NumInstances = static_cast<Uint32>(m_Scene.Objects.size());
    bool         Update       = true;

    // Create scratch buffer
    if (!m_Scene.TLASScratchBuffer)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "TLAS Scratch Buffer";
        BuffDesc.Usage     = USAGE_DEFAULT;
        BuffDesc.BindFlags = BIND_RAY_TRACING;
        BuffDesc.Size      = std::max(m_Scene.TLAS->GetScratchBufferSizes().Build, m_Scene.TLAS->GetScratchBufferSizes().Update);
        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_Scene.TLASScratchBuffer);
        Update = false; // this is the first build
    }

    // Create instance buffer
    if (!m_Scene.TLASInstancesBuffer)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "TLAS Instance Buffer";
        BuffDesc.Usage     = USAGE_DEFAULT;
        BuffDesc.BindFlags = BIND_RAY_TRACING;
        BuffDesc.Size      = Uint64{TLAS_INSTANCE_DATA_SIZE} * Uint64{NumInstances};
        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_Scene.TLASInstancesBuffer);
    }

    // Setup instances
    std::vector<TLASBuildInstanceData> Instances(NumInstances);
    std::vector<String>                InstanceNames(NumInstances);
    for (Uint32 i = 0; i < NumInstances; ++i)
    {
        const auto& Obj      = m_Scene.Objects[i];
        auto&       Inst     = Instances[i];
        auto&       Name     = InstanceNames[i];
        const auto& Mesh     = m_Scene.Meshes[Obj.MeshId];
        const auto  ModelMat = Obj.ModelMat.Transpose();

        Name = Mesh.Name + " Instance (" + std::to_string(i) + ")";

        Inst.InstanceName = Name.c_str();
        Inst.pBLAS        = Mesh.BLAS;
        Inst.Mask         = 0xFF;

        // CustomId will be read in shader by RayQuery::CommittedInstanceID()
        Inst.CustomId = i;

        Inst.Transform.SetRotation(ModelMat.Data(), 4);
        Inst.Transform.SetTranslation(ModelMat.m30, ModelMat.m31, ModelMat.m32);
    }

    // Build  TLAS
    BuildTLASAttribs Attribs;
    Attribs.pTLAS  = m_Scene.TLAS;
    Attribs.Update = Update;

    // Scratch buffer will be used to store temporary data during TLAS build or update.
    // Previous content in the scratch buffer will be discarded.
    Attribs.pScratchBuffer = m_Scene.TLASScratchBuffer;

    // Instance buffer will store instance data during TLAS build or update.
    // Previous content in the instance buffer will be discarded.
    Attribs.pInstanceBuffer = m_Scene.TLASInstancesBuffer;

    // Instances will be converted to the format that is required by the graphics driver and copied to the instance buffer.
    Attribs.pInstances    = Instances.data();
    Attribs.InstanceCount = NumInstances;

    // Allow engine to change resource states.
    Attribs.TLASTransitionMode           = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.BLASTransitionMode           = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.InstanceBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.ScratchBufferTransitionMode  = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pImmediateContext->BuildTLAS(Attribs);
}

void Tutorial22_HybridRendering::CreateScene()
{
    uint2                              CubeMaterialRange;
    Uint32                             GroundMaterial;
    std::vector<HLSL::MaterialAttribs> Materials;
    CreateSceneMaterials(CubeMaterialRange, GroundMaterial, Materials);
    CreateSceneObjects(CubeMaterialRange, GroundMaterial);
    CreateSceneAccelStructs();

    // Create buffer for object attribs
    {
        BufferDesc BuffDesc;
        BuffDesc.Name              = "Object attribs buffer";
        BuffDesc.Usage             = USAGE_DEFAULT;
        BuffDesc.BindFlags         = BIND_SHADER_RESOURCE;
        BuffDesc.Size              = static_cast<Uint64>(sizeof(m_Scene.Objects[0]) * m_Scene.Objects.size());
        BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
        BuffDesc.ElementByteStride = sizeof(m_Scene.Objects[0]);
        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_Scene.ObjectAttribsBuffer);
    }

    // Create and initialize buffer for material attribs
    {
        BufferDesc BuffDesc;
        BuffDesc.Name              = "Material attribs buffer";
        BuffDesc.Usage             = USAGE_DEFAULT;
        BuffDesc.BindFlags         = BIND_SHADER_RESOURCE;
        BuffDesc.Size              = static_cast<Uint64>(sizeof(Materials[0]) * Materials.size());
        BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
        BuffDesc.ElementByteStride = sizeof(Materials[0]);

        BufferData BuffData{Materials.data(), BuffDesc.Size};
        m_pDevice->CreateBuffer(BuffDesc, &BuffData, &m_Scene.MaterialAttribsBuffer);
    }

    // Create dynamic buffer for scene object constants (unique for each draw call)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Global constants buffer";
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        BuffDesc.Size           = sizeof(HLSL::ObjectConstants);
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_Scene.ObjectConstants);
    }
}

void Tutorial22_HybridRendering::CreateRasterizationPSO(IShaderSourceInputStreamFactory* pShaderSourceFactory)
{
    // Create PSO for rendering to GBuffer

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("NUM_TEXTURES", static_cast<Uint32>(m_Scene.Textures.size()));
    Macros.AddShaderMacro("NUM_SAMPLERS", static_cast<Uint32>(m_Scene.Samplers.size()));

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Rasterization PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 2;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_ColorTargetFormat;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[1]                = m_NormalTargetFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_DepthTargetFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = m_ShaderCompiler;
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.Macros                     = Macros;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Rasterization VS";
        ShaderCI.FilePath        = "Rasterization.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Rasterization PS";
        ShaderCI.FilePath        = "Rasterization.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    LayoutElement LayoutElems[] =
        {
            LayoutElement{0, 0, 3, VT_FLOAT32, False},
            LayoutElement{1, 0, 3, VT_FLOAT32, False},
            LayoutElement{2, 0, 2, VT_FLOAT32, False} //
        };
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType        = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableMergeStages = SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL;

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_RasterizationPSO);

    m_RasterizationPSO->CreateShaderResourceBinding(&m_RasterizationSRB);
    m_RasterizationSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Constants")->Set(m_Constants);
    m_RasterizationSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_ObjectConst")->Set(m_Scene.ObjectConstants);
    m_RasterizationSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_ObjectAttribs")->Set(m_Scene.ObjectAttribsBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    m_RasterizationSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MaterialAttribs")->Set(m_Scene.MaterialAttribsBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

    // Bind textures
    {
        const auto                  NumTextures = static_cast<Uint32>(m_Scene.Textures.size());
        std::vector<IDeviceObject*> ppTextures(NumTextures);
        for (Uint32 i = 0; i < NumTextures; ++i)
            ppTextures[i] = m_Scene.Textures[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_RasterizationSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Textures")->SetArray(ppTextures.data(), 0, NumTextures);
    }

    // Bind samplers
    {
        const auto                  NumSamplers = static_cast<Uint32>(m_Scene.Samplers.size());
        std::vector<IDeviceObject*> ppSamplers(NumSamplers);
        for (Uint32 i = 0; i < NumSamplers; ++i)
            ppSamplers[i] = m_Scene.Samplers[i];
        m_RasterizationSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Samplers")->SetArray(ppSamplers.data(), 0, NumSamplers);
    }
}

void Tutorial22_HybridRendering::CreatePostProcessPSO(IShaderSourceInputStreamFactory* pShaderSourceFactory)
{
    // Create PSO for post process pass

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Post process PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    PSOCreateInfo.GraphicsPipeline.NumRenderTargets                  = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                     = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = false;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;

    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = m_ShaderCompiler;
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Post process VS";
        ShaderCI.FilePath        = "PostProcess.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Post process PS";
        ShaderCI.FilePath        = "PostProcess.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_PostProcessPSO);
}

void Tutorial22_HybridRendering::CreateRayTracingPSO(IShaderSourceInputStreamFactory* pShaderSourceFactory)
{
    // Create compute shader that performs inline ray tracing

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("NUM_TEXTURES", static_cast<Uint32>(m_Scene.Textures.size()));
    Macros.AddShaderMacro("NUM_SAMPLERS", static_cast<Uint32>(m_Scene.Samplers.size()));

    ComputePipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;

    const auto NumTextures = static_cast<Uint32>(m_Scene.Textures.size());
    const auto NumSamplers = static_cast<Uint32>(m_Scene.Samplers.size());

    // Split the resources of the ray tracing PSO into two groups.
    // The first group will contain scene resources. These resources
    // may be bound only once.
    // The second group will contain screen-dependent resources.
    // These resources will need to be bound every time the screen is resized.

    // Resource signature for scene resources
    {
        PipelineResourceSignatureDesc PRSDesc;
        PRSDesc.Name = "Ray tracing scene resources";

        // clang-format off
        const PipelineResourceDesc Resources[] =
        {
            {SHADER_TYPE_COMPUTE, "g_TLAS",            1,           SHADER_RESOURCE_TYPE_ACCEL_STRUCT},
            {SHADER_TYPE_COMPUTE, "g_Constants",       1,           SHADER_RESOURCE_TYPE_CONSTANT_BUFFER},
            {SHADER_TYPE_COMPUTE, "g_ObjectAttribs",   1,           SHADER_RESOURCE_TYPE_BUFFER_SRV},
            {SHADER_TYPE_COMPUTE, "g_MaterialAttribs", 1,           SHADER_RESOURCE_TYPE_BUFFER_SRV},
            {SHADER_TYPE_COMPUTE, "g_VertexBuffer",    1,           SHADER_RESOURCE_TYPE_BUFFER_SRV},
            {SHADER_TYPE_COMPUTE, "g_IndexBuffer",     1,           SHADER_RESOURCE_TYPE_BUFFER_SRV},
            {SHADER_TYPE_COMPUTE, "g_Textures",        NumTextures, SHADER_RESOURCE_TYPE_TEXTURE_SRV},
            {SHADER_TYPE_COMPUTE, "g_Samplers",        NumSamplers, SHADER_RESOURCE_TYPE_SAMPLER}
        };
        // clang-format on
        PRSDesc.BindingIndex = 0;
        PRSDesc.Resources    = Resources;
        PRSDesc.NumResources = _countof(Resources);
        m_pDevice->CreatePipelineResourceSignature(PRSDesc, &m_pRayTracingSceneResourcesSign);
        VERIFY_EXPR(m_pRayTracingSceneResourcesSign);
    }

    // Resource signature for screen resources
    {
        PipelineResourceSignatureDesc PRSDesc;
        PRSDesc.Name = "Ray tracing screen resources";

        // clang-format off
        const PipelineResourceDesc Resources[] =
        {
            {SHADER_TYPE_COMPUTE, "g_RayTracedTex",   1, SHADER_RESOURCE_TYPE_TEXTURE_UAV},
            {SHADER_TYPE_COMPUTE, "g_GBuffer_Normal", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV},
            {SHADER_TYPE_COMPUTE, "g_GBuffer_Depth",  1, SHADER_RESOURCE_TYPE_TEXTURE_SRV}
        };
        // clang-format on
        PRSDesc.BindingIndex = 1;
        PRSDesc.Resources    = Resources;
        PRSDesc.NumResources = _countof(Resources);
        m_pDevice->CreatePipelineResourceSignature(PRSDesc, &m_pRayTracingScreenResourcesSign);
        VERIFY_EXPR(m_pRayTracingScreenResourcesSign);
    }

    IPipelineResourceSignature* ppSignatures[]{m_pRayTracingSceneResourcesSign, m_pRayTracingScreenResourcesSign};
    PSOCreateInfo.ppResourceSignatures    = ppSignatures;
    PSOCreateInfo.ResourceSignaturesCount = _countof(ppSignatures);

    ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.EntryPoint                 = "CSMain";
    ShaderCI.Macros                     = Macros;

    if (m_pDevice->GetDeviceInfo().IsMetalDevice())
    {
        // HLSL and MSL are very similar, so we can use the same code for all
        // platforms, with some macros help.
        ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL;
    }
    else
    {
        // Inline ray tracing requires shader model 6.5
        // Only DXC can compile HLSL for ray tracing.
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
        ShaderCI.HLSLVersion    = {6, 5};
    }

    ShaderCI.Desc.Name = "Ray tracing CS";
    ShaderCI.FilePath  = "RayTracing.csh";
    if (m_pDevice->GetDeviceInfo().IsMetalDevice())
    {
        // The shader uses macros that are not supported by MSL parser in Metal backend
        ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_SKIP_REFLECTION;
    }
    RefCntAutoPtr<IShader> pCS;
    m_pDevice->CreateShader(ShaderCI, &pCS);
    PSOCreateInfo.pCS = pCS;

    PSOCreateInfo.PSODesc.Name = "Ray tracing PSO";
    m_pDevice->CreateComputePipelineState(PSOCreateInfo, &m_RayTracingPSO);
    VERIFY_EXPR(m_RayTracingPSO);

    // Initialize SRB containing scene resources
    m_pRayTracingSceneResourcesSign->CreateShaderResourceBinding(&m_RayTracingSceneSRB);
    m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_TLAS")->Set(m_Scene.TLAS);
    m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Constants")->Set(m_Constants);
    m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_ObjectAttribs")->Set(m_Scene.ObjectAttribsBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_MaterialAttribs")->Set(m_Scene.MaterialAttribsBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

    // Bind mesh geometry buffers. All meshes use shared vertex and index buffers.
    m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_VertexBuffer")->Set(m_Scene.Meshes[0].VertexBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_IndexBuffer")->Set(m_Scene.Meshes[0].IndexBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

    // Bind material textures
    {
        std::vector<IDeviceObject*> ppTextures(NumTextures);
        for (Uint32 i = 0; i < NumTextures; ++i)
            ppTextures[i] = m_Scene.Textures[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Textures")->SetArray(ppTextures.data(), 0, NumTextures);
    }

    // Bind samplers
    {
        std::vector<IDeviceObject*> ppSamplers(NumSamplers);
        for (Uint32 i = 0; i < NumSamplers; ++i)
            ppSamplers[i] = m_Scene.Samplers[i];
        m_RayTracingSceneSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_Samplers")->SetArray(ppSamplers.data(), 0, NumSamplers);
    }
}

void Tutorial22_HybridRendering::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    // RayTracing feature indicates that some of ray tracing functionality is supported.
    // Acceleration structures are always supported if RayTracing feature is enabled.
    // Inline ray tracing may be unsupported by old DirectX 12 drivers or if this feature is not supported by Vulkan.
    if ((m_pDevice->GetAdapterInfo().RayTracing.CapFlags & RAY_TRACING_CAP_FLAG_INLINE_RAY_TRACING) == 0)
    {
        UNSUPPORTED("Inline ray tracing is not supported by device");
        return;
    }

    // Setup camera.
    m_Camera.SetPos(float3{-15.7f, 3.7f, -5.8f});
    m_Camera.SetRotation(17.7f, -0.1f);
    m_Camera.SetRotationSpeed(0.005f);
    m_Camera.SetMoveSpeed(5.f);
    m_Camera.SetSpeedUpScales(5.f, 10.f);

    CreateScene();

    // Create buffer for constants that is shared between all PSOs
    {
        BufferDesc BuffDesc;
        BuffDesc.Name      = "Global constants buffer";
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Size      = sizeof(HLSL::GlobalConstants);
        m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_Constants);
    }

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

    CreateRasterizationPSO(pShaderSourceFactory);
    CreatePostProcessPSO(pShaderSourceFactory);
    CreateRayTracingPSO(pShaderSourceFactory);
}

void Tutorial22_HybridRendering::ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs)
{
    SampleBase::ModifyEngineInitInfo(Attribs);

    // Require ray tracing feature.
    Attribs.EngineCI.Features.RayTracing = DEVICE_FEATURE_STATE_ENABLED;
}

void Tutorial22_HybridRendering::Render()
{
    // Update constants
    {
        const auto ViewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();

        HLSL::GlobalConstants GConst;
        GConst.ViewProj     = ViewProj.Transpose();
        GConst.ViewProjInv  = ViewProj.Inverse().Transpose();
        GConst.LightDir     = normalize(-m_LightDir);
        GConst.CameraPos    = float4(m_Camera.GetPos(), 0.f);
        GConst.DrawMode     = m_DrawMode;
        GConst.MaxRayLength = 100.f;
        GConst.AmbientLight = 0.002f;

        // Constantes que cree para la
        GConst.FlashlightPos       = float4(m_Camera.GetPos(), 0.0f);
        GConst.FlashlightDir       = float4(m_Camera.GetWorldAhead(), 10.0f);
        GConst.FlashlightRange     = 30.0f;
        GConst.FlashlightConeAngle = cos(PI_F * 20.0f / 180.0f);
        GConst.FlashlightIntensity = m_FlashlightEnabled ? 0.5f : 0.0f;

        m_pImmediateContext->UpdateBuffer(m_Constants, 0, static_cast<Uint32>(sizeof(GConst)), &GConst, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Update transformation for scene objects
        m_pImmediateContext->UpdateBuffer(m_Scene.ObjectAttribsBuffer, 0, static_cast<Uint32>(sizeof(HLSL::ObjectAttribs) * m_Scene.Objects.size()),
                                          m_Scene.Objects.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    UpdateTLAS();

    // Rasterization pass
    {
        ITextureView* RTVs[] = //
            {
                m_GBuffer.Color->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
                m_GBuffer.Normal->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) //
            };
        ITextureView* pDSV = m_GBuffer.Depth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pImmediateContext->SetRenderTargets(_countof(RTVs), RTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // All transitions for render targets happened in SetRenderTargets()
        const float ClearColor[4] = {};
        m_pImmediateContext->ClearRenderTarget(RTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_NONE);
        m_pImmediateContext->ClearRenderTarget(RTVs[1], ClearColor, RESOURCE_STATE_TRANSITION_MODE_NONE);
        m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_NONE);

        m_pImmediateContext->SetPipelineState(m_RasterizationPSO);
        m_pImmediateContext->CommitShaderResources(m_RasterizationSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        for (auto& ObjInst : m_Scene.ObjectInstances)
        {
            auto&        Mesh      = m_Scene.Meshes[ObjInst.MeshInd];
            IBuffer*     VBs[]     = {Mesh.VertexBuffer};
            const Uint64 Offsets[] = {Mesh.FirstVertex * sizeof(HLSL::Vertex)};

            m_pImmediateContext->SetVertexBuffers(0, _countof(VBs), VBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
            m_pImmediateContext->SetIndexBuffer(Mesh.IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            {
                MapHelper<HLSL::ObjectConstants> ObjConstants{m_pImmediateContext, m_Scene.ObjectConstants, MAP_WRITE, MAP_FLAG_DISCARD};
                ObjConstants->ObjectAttribsOffset = ObjInst.ObjectAttribsOffset;
            }

            DrawIndexedAttribs drawAttribs;
            drawAttribs.NumIndices         = Mesh.NumIndices;
            drawAttribs.NumInstances       = ObjInst.NumObjects;
            drawAttribs.FirstIndexLocation = Mesh.FirstIndex;
            drawAttribs.IndexType          = VT_UINT32;
            drawAttribs.Flags              = DRAW_FLAG_VERIFY_ALL;
            m_pImmediateContext->DrawIndexed(drawAttribs);
        }
    }

    // Ray tracing pass
    {
        DispatchComputeAttribs dispatchAttribs;
        dispatchAttribs.MtlThreadGroupSizeX = m_BlockSize.x;
        dispatchAttribs.MtlThreadGroupSizeY = m_BlockSize.y;
        dispatchAttribs.MtlThreadGroupSizeZ = 1;

        const auto& TexDesc               = m_GBuffer.Color->GetDesc();
        dispatchAttribs.ThreadGroupCountX = (TexDesc.Width / m_BlockSize.x);
        dispatchAttribs.ThreadGroupCountY = (TexDesc.Height / m_BlockSize.y);

        m_pImmediateContext->SetPipelineState(m_RayTracingPSO);
        m_pImmediateContext->CommitShaderResources(m_RayTracingSceneSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->CommitShaderResources(m_RayTracingScreenSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->DispatchCompute(dispatchAttribs);
    }

    // Post process pass
    {
        auto*       pRTV          = m_pSwapChain->GetCurrentBackBufferRTV();
        const float ClearColor[4] = {};
        m_pImmediateContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        m_pImmediateContext->SetPipelineState(m_PostProcessPSO);
        m_pImmediateContext->CommitShaderResources(m_PostProcessSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        m_pImmediateContext->SetVertexBuffers(0, 0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pImmediateContext->SetIndexBuffer(nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_NONE);

        m_pImmediateContext->Draw(DrawAttribs{3, DRAW_FLAG_VERIFY_ALL});
    }
}

void Tutorial22_HybridRendering::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();
    if (m_ShowStartScreen || m_ShowControlsScreen)
        return;

    if (ImGui::IsKeyReleased(ImGuiKey_F))
    {
        m_FlashlightEnabled = !m_FlashlightEnabled;
    }

    const float dt = static_cast<float>(ElapsedTime);


    if (m_DamageEffectTimer > 0.0f)
    {
        m_DamageEffectTimer -= dt;
    }

    float3 PrevCameraPos = m_Camera.GetPos();
    m_Camera.Update(m_InputController, dt);

    if (!m_Scene.DynamicObjects.empty())
    {
        auto& DynObj = m_Scene.DynamicObjects[0];
        auto& Obj    = m_Scene.Objects[DynObj.ObjectAttribsIndex];

        float3   camPos      = m_Camera.GetPos();
        float4x4 modelMat    = Obj.ModelMat.Transpose();
        float3   monsterPos  = float3{modelMat[3][0], modelMat[3][1], modelMat[3][2]};
        float3   dirToCamera = normalize(camPos - monsterPos);

        float distance = length(camPos - monsterPos);
        if (distance > 1.5f)
        {
            monsterPos += dirToCamera * 3.0f * dt;
            monsterPos.y = 3.0f;
        }

        Obj.ModelMat = (float4x4::Scale(1.0f, 1.0f, 1.0f) *
                        float4x4::Translation(monsterPos))
                           .Transpose();
        Obj.NormalMat = float4x3{Obj.ModelMat};
    }

    // Verificar colisión con el monstruo
    float3 playerPos  = m_Camera.GetPos();
    float3 monsterPos = float3(m_Scene.Objects[m_Scene.DynamicObjects[0].ObjectAttribsIndex].ModelMat.Transpose().m30,
                               m_Scene.Objects[m_Scene.DynamicObjects[0].ObjectAttribsIndex].ModelMat.Transpose().m31,
                               m_Scene.Objects[m_Scene.DynamicObjects[0].ObjectAttribsIndex].ModelMat.Transpose().m32);
    float  distance   = length(playerPos - monsterPos);

    if (distance < 2.0f && !m_IsGameOver)
    {
        m_TimeSinceLastDamage += dt;

        while (m_TimeSinceLastDamage >= m_DamageCooldown)
        {
            m_Health = std::max(0, m_Health - 25);
            m_TimeSinceLastDamage -= m_DamageCooldown;

            m_DamageEffectTimer      = 0.3f;
            m_PostDamageOverlayAlpha = 1.0f;
            m_PostDamageOverlayTimer = 0.0f;

            if (m_Health <= 0)
            {
                m_IsGameOver = true;
                break;
            }
        }
    }
    else
    {
        m_TimeSinceLastDamage = 0.0f;
    }

    if (m_PostDamageOverlayAlpha > 0.0f)
    {
        m_PostDamageOverlayTimer += dt;
        float t                  = m_PostDamageOverlayTimer / m_PostDamageOverlayDuration;
        m_PostDamageOverlayAlpha = std::max(0.0f, 1.0f - t);
    }


    float3 NewCamPos = m_Camera.GetPos();
    HandleCollisions(NewCamPos, 0.5f);
    HandleKeyCollection(NewCamPos, 0.5f);
    if (m_ShowUnlockMsg)
    {
        m_UnlockMsgTimer += dt;
        if (m_UnlockMsgTimer >= m_UnlockMsgTime)
            m_ShowUnlockMsg = false;
    }

    TryOpenDoors();

    for (auto& door : m_Doors)
    {
        if (!door.Rising) continue;

        door.RiseTimer += dt;
        float offsetY = door.RiseTimer * door.RiseSpeed;

        // Corregir cálculo de matriz
        float4x4 riseTrans                        = float4x4::Translation(0.0f, offsetY, 0.0f).Transpose();
        m_Scene.Objects[door.ObjectIdx].ModelMat  = (door.OriginalMat * riseTrans);
        m_Scene.Objects[door.ObjectIdx].NormalMat = float4x3{m_Scene.Objects[door.ObjectIdx].ModelMat};

        if (offsetY > 3.0f)
        {
            MazeWalls[door.WallIdx]                  = {{0, 0, 0}, {0, 0, 0}};
            m_Scene.Objects[door.ObjectIdx].ModelMat = float4x4::Scale(0, 0, 0).Transpose();
            door.Rising                              = false;
        }
    }

    NewCamPos.y = std::max(0.1f, std::min(NewCamPos.y, 60.0f));
    m_Camera.SetPos(NewCamPos);

    // Restrict camera movement
    float3 Pos = m_Camera.GetPos();

    // Fijar la altura (Y) de la cámara
    Pos.y = 3.0f;

    const float3 MinXYZ{-100.f, 0.1f, -100.f};
    const float3 MaxXYZ{+100.f, 60.f, 100.f};

    Pos = clamp(Pos, MinXYZ, MaxXYZ);

    m_Camera.SetPos(Pos);
    m_Camera.Update(m_InputController, 0);


    // Update dynamic objects
    float RotationSpeed = 0.15f;
    for (auto& DynObj : m_Scene.DynamicObjects)
    {
        auto& Obj      = m_Scene.Objects[DynObj.ObjectAttribsIndex];
        auto  ModelMat = Obj.ModelMat.Transpose();
        Obj.ModelMat   = (float4x4::RotationY(PI_F * dt * RotationSpeed) * ModelMat).Transpose();
        Obj.NormalMat  = float4x3{Obj.ModelMat};

        RotationSpeed *= 1.5f;
    }
}

void Tutorial22_HybridRendering::WindowResize(Uint32 Width, Uint32 Height)
{
    if (Width == 0 || Height == 0)
        return;

    // Round to multiple of m_BlockSize
    Width  = AlignUp(Width, m_BlockSize.x);
    Height = AlignUp(Height, m_BlockSize.y);

    // Update projection matrix.
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    m_Camera.SetProjAttribs(0.5f, 1000.f, AspectRatio, PI_F / 4.f,
                            m_pSwapChain->GetDesc().PreTransform, m_pDevice->GetDeviceInfo().NDC.MinZ == -1);

    // Check if the image needs to be recreated.
    if (m_GBuffer.Color != nullptr &&
        m_GBuffer.Color->GetDesc().Width == Width &&
        m_GBuffer.Color->GetDesc().Height == Height)
        return;

    m_GBuffer = {};

    // Create window-size G-buffer textures.
    TextureDesc RTDesc;
    RTDesc.Name      = "GBuffer Color";
    RTDesc.Type      = RESOURCE_DIM_TEX_2D;
    RTDesc.Width     = Width;
    RTDesc.Height    = Height;
    RTDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    RTDesc.Format    = m_ColorTargetFormat;
    m_pDevice->CreateTexture(RTDesc, nullptr, &m_GBuffer.Color);

    RTDesc.Name      = "GBuffer Normal";
    RTDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    RTDesc.Format    = m_NormalTargetFormat;
    m_pDevice->CreateTexture(RTDesc, nullptr, &m_GBuffer.Normal);

    RTDesc.Name      = "GBuffer Depth";
    RTDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
    RTDesc.Format    = m_DepthTargetFormat;
    m_pDevice->CreateTexture(RTDesc, nullptr, &m_GBuffer.Depth);

    RTDesc.Name      = "Ray traced shadow & reflection";
    RTDesc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    RTDesc.Format    = m_RayTracedTexFormat;
    m_RayTracedTex.Release();
    m_pDevice->CreateTexture(RTDesc, nullptr, &m_RayTracedTex);


    // Create post-processing SRB
    {
        m_PostProcessSRB.Release();
        m_PostProcessPSO->CreateShaderResourceBinding(&m_PostProcessSRB);
        m_PostProcessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Constants")->Set(m_Constants);
        m_PostProcessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBuffer_Color")->Set(m_GBuffer.Color->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_PostProcessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBuffer_Normal")->Set(m_GBuffer.Normal->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_PostProcessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBuffer_Depth")->Set(m_GBuffer.Depth->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_PostProcessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_RayTracedTex")->Set(m_RayTracedTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    }

    // Create ray-tracing screen SRB
    {
        m_RayTracingScreenSRB.Release();
        m_pRayTracingScreenResourcesSign->CreateShaderResourceBinding(&m_RayTracingScreenSRB);
        m_RayTracingScreenSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_RayTracedTex")->Set(m_RayTracedTex->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
        m_RayTracingScreenSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_GBuffer_Depth")->Set(m_GBuffer.Depth->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_RayTracingScreenSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_GBuffer_Normal")->Set(m_GBuffer.Normal->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    }
}

void Tutorial22_HybridRendering::UpdateUI()
{
    // Fullscreen overlay message (puertas desbloqueadas)
    if (m_ShowUnlockMsg)
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin("##FullscreenOverlay", nullptr, overlayFlags);
        {
            const char* msg = "Las puertas han sido desbloqueadas!";
            ImGui::SetWindowFontScale(2.5f);
            ImVec2 textSize = ImGui::CalcTextSize(msg);

            float x = (vp->Size.x - textSize.x) * 0.5f;
            float y = (vp->Size.y - textSize.y) * 0.5f;
            ImGui::SetCursorPos(ImVec2{x, y});
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", msg);
            ImGui::SetWindowFontScale(1.0f);
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    if (m_DamageEffectTimer > 0.0f && !m_IsGameOver)
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);

        float alpha = 0.3f * (m_DamageEffectTimer / 0.3f);
        ImGui::SetNextWindowBgAlpha(alpha);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("DamageEffect", nullptr, flags))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(
                ImVec2(vp->Pos.x, vp->Pos.y),
                ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y),
                IM_COL32(255, 0, 0, static_cast<int>(100 * alpha)));
        }
        ImGui::End();
    }

    if (m_PostDamageOverlayAlpha > 0.0f && !m_IsGameOver)
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground;

        if (ImGui::Begin("PostDamageBlurOverlay", nullptr, flags))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(
                ImVec2(vp->Pos.x, vp->Pos.y),
                ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y),
                IM_COL32(180, 0, 0, static_cast<int>(150 * m_PostDamageOverlayAlpha)));
        }
        ImGui::End();
    }

    if (m_IsGameOver)
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowBgAlpha(0.85f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar;

        if (ImGui::Begin("GameOverScreen", nullptr, flags))
        {
            ImDrawList* draw_list   = ImGui::GetWindowDrawList();
            ImVec2      window_pos  = ImGui::GetWindowPos();
            ImVec2      window_size = ImGui::GetWindowSize();
            draw_list->AddRectFilled(window_pos, ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y), IM_COL32(10, 0, 0, 200));

            const char* msg = "GAME OVER";
            ImGui::SetWindowFontScale(4.0f);
            ImVec2 textSize = ImGui::CalcTextSize(msg);
            ImGui::SetCursorPosX((window_size.x - textSize.x) * 0.5f);
            ImGui::SetCursorPosY((window_size.y - textSize.y) * 0.4f);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", msg);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(170, 30, 30, 200));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(200, 50, 50, 255));

            const char* btnText = "REINICIAR JUEGO";
            ImGui::SetWindowFontScale(2.0f);
            textSize = ImGui::CalcTextSize(btnText);
            ImGui::SetCursorPosX((window_size.x - textSize.x) * 0.5f);
            ImGui::SetCursorPosY((window_size.y - textSize.y) * 0.6f);

            if (ImGui::Button(btnText, ImVec2(textSize.x + 40.0f, textSize.y + 20.0f)))
            {
                m_Health            = 100;
                m_IsGameOver        = false;
                m_DamageEffectTimer = 0.0f;
                m_Camera.SetPos(float3{-15.7f, 3.7f, -5.8f});
            }

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();

            ImGui::End();
        }
    }

    if (m_ShowStartScreen)
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowBgAlpha(0.95f);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("StartScreen", nullptr, flags))
        {
            // Fondo degradado
            ImDrawList* draw_list   = ImGui::GetWindowDrawList();
            ImVec2      window_pos  = ImGui::GetWindowPos();
            ImVec2      window_size = ImGui::GetWindowSize();
            draw_list->AddRectFilledMultiColor(
                window_pos,
                ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y),
                IM_COL32(12, 45, 70, 200),
                IM_COL32(8, 30, 48, 200),
                IM_COL32(8, 30, 48, 200),
                IM_COL32(12, 45, 70, 200));

            // Título
            ImGui::SetWindowFontScale(3.0f);
            ImVec2 textSize = ImGui::CalcTextSize("BIENVENIDO");
            ImGui::SetCursorPosX((window_size.x - textSize.x) * 0.5f);
            ImGui::SetCursorPosY(window_size.y * 0.3f);
            ImGui::TextColored(ImColor(220, 220, 250), "BIENVENIDO");


            ImGui::SetCursorPosY(window_size.y * 0.4f);
            ImGui::SetCursorPosY(window_size.y * 0.45f);

            // Botones
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 15));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 80, 120, 200));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 100, 140, 220));

            ImGui::SetWindowFontScale(1.5f);
            ImGui::SetCursorPosX((window_size.x - 250) * 0.5f);
            if (ImGui::Button("VER CONTROLES", ImVec2(250, 60)))
            {
                m_ShowControlsScreen = true;
                m_ShowStartScreen    = false;
            }

            ImGui::SetCursorPosX((window_size.x - 250) * 0.5f);
            if (ImGui::Button("INICIAR JUEGO", ImVec2(250, 60)))
            {
                m_ShowStartScreen = false;
            }

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
        }
        ImGui::End();
        return;
    }

    if (m_ShowControlsScreen)
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowBgAlpha(0.95f);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("ControlsScreen", nullptr, flags))
        {
            // Fondo
            ImDrawList* draw_list   = ImGui::GetWindowDrawList();
            ImVec2      window_pos  = ImGui::GetWindowPos();
            ImVec2      window_size = ImGui::GetWindowSize();
            draw_list->AddRectFilled(
                window_pos,
                ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y),
                IM_COL32(18, 35, 45, 200));

            // Título
            ImGui::SetWindowFontScale(2.5f);
            ImVec2 textSize = ImGui::CalcTextSize("CONTROLES");
            ImGui::SetCursorPosX((window_size.x - textSize.x) * 0.5f);
            ImGui::SetCursorPosY(window_size.y * 0.1f);
            ImGui::TextColored(ImColor(180, 200, 220), "CONTROLES");

            // Panel de controles
            ImGui::SetWindowFontScale(1.2f);
            ImGui::SetCursorPos(ImVec2(window_size.x * 0.25f, window_size.y * 0.25f));
            if (ImGui::BeginChild("##ControlsPanel", ImVec2(window_size.x * 0.5f, window_size.y * 0.5f), true))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 20));

                ImGui::Bullet();
                ImGui::TextColored(ImColor(100, 180, 255), "Movimiento:");
                ImGui::Indent(20);
                ImGui::Text("WASD - Desplazamiento");
                ImGui::Text("Shift - Correr");
                ImGui::Unindent(20);

                ImGui::Spacing();

                ImGui::Bullet();
                ImGui::TextColored(ImColor(100, 180, 255), "Acciones:");
                ImGui::Indent(20);
                ImGui::Text("F - Linterna");
                ImGui::Text("Mouse - Rotar camara");
                ImGui::Unindent(20);

                ImGui::PopStyleVar();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(window_size.x * 0.3f, window_size.y * 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 100, 140, 200));

            if (ImGui::Button("VOLVER", ImVec2(150, 40)))
            {
                m_ShowControlsScreen = false;
                m_ShowStartScreen    = true;
            }

            ImGui::SameLine(window_size.x * 0.55f);

            if (ImGui::Button("JUGAR", ImVec2(150, 40)))
            {
                m_ShowControlsScreen = false;
            }

            ImGui::PopStyleColor();
        }
        ImGui::End();
        return;
    }

    // Ventana de configuración
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Separator();

        // VIDAS
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 3));

        // Colores personalizados para la barra
        const ImU32 bgColor     = IM_COL32(30, 60, 30, 255);
        const ImU32 fillColor   = IM_COL32(50, 200, 50, 255);
        const ImU32 borderColor = IM_COL32(20, 40, 20, 255);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, bgColor);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, fillColor);

        ImVec2      barSize(200, 24);
        float       healthPercent = m_Health / 100.0f;
        std::string healthText    = std::to_string(m_Health) + "%";

        // Título de salud
        ImGui::TextColored(ImColor(200, 255, 200), "SALUD");

        // Barra de vida
        ImGui::BeginGroup();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImGui::ProgressBar(healthPercent, barSize, "");

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRect(cursorPos, ImVec2(cursorPos.x + barSize.x, cursorPos.y + barSize.y), borderColor, 3.0f);

        // Texto del porcentaje
        ImVec2 textSize = ImGui::CalcTextSize(healthText.c_str());
        ImVec2 textPos  = ImVec2(
            cursorPos.x + (barSize.x - textSize.x) * 0.5f,
            cursorPos.y + (barSize.y - textSize.y) * 0.5f);
        draw_list->AddText(textPos, IM_COL32(255, 255, 255, 255), healthText.c_str());

        ImGui::EndGroup();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        // Tiempo de daño
        if (m_TimeSinceLastDamage > 0 && !m_IsGameOver)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 255, 200, 255));
            ImGui::Text("Próximo daño en: %.1fs", m_DamageCooldown - m_TimeSinceLastDamage);
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // LINTERNA
        ImGui::BeginGroup();
        ImGui::TextColored(ImColor(200, 255, 200), "LINTERNA");

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));

        const ImVec4 btnColor = m_FlashlightEnabled ?
            ImVec4(0.2f, 0.7f, 0.2f, 0.9f) : // Verde cuando está activada
            ImVec4(0.7f, 0.2f, 0.2f, 0.9f);  // Rojo cuando está apagada

        ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(btnColor.x + 0.1f, btnColor.y + 0.1f, btnColor.z + 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(btnColor.x * 0.8f, btnColor.y * 0.8f, btnColor.z * 0.8f, 1.0f));

        if (ImGui::Button(m_FlashlightEnabled ? " ACTIVADA " : " APAGADA ", ImVec2(120, 30)))
        {
            m_FlashlightEnabled = !m_FlashlightEnabled;
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 220, 180, 255));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
        ImGui::Text("(Presiona F para alternar)");
        ImGui::PopStyleColor();

        ImGui::EndGroup();
    }
    ImGui::End();
}
} // namespace Diligent
