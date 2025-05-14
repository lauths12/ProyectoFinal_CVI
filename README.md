# 🎮 Backroom
---

## 🧩 Descripción General

Este proyecto es un videojuego en primera persona que busca recrear una experiencia inmersiva de terror y exploración, inspirado en lo conocido como *Backrooms*, pero con modificaciones y elementos propios, como acertijos y una jugabilidad tipo *Escape Room*.

El jugador se mueve por un laberinto oscuro equipado únicamente con una linterna, resolviendo acertijos e intentando evitar ser atrapado por entidades misteriosas.

Este proyecto Fue desarrollado como una forma de integrar y aplicar conceptos clave aprendidos en el curso, incluyendo texturizado, iluminación dinámica y renderizado híbrido con ray tracing.

---

## 📦 Entrega 1 – Base del Laberinto y Sistema de Colisiones
![image](https://github.com/user-attachments/assets/fdd866f2-a518-4d7c-b3a1-d70ca2d64574)

### 🧱 Generación del Laberinto

* Se implementó un generador procedural de laberintos basado en el **algoritmo de Prim**, que garantiza caminos únicos sin ciclos.
* Se adaptó la función `CreateSceneObjects` para instanciar correctamente muros y pasillos de acuerdo al laberinto generado.

![image](https://github.com/user-attachments/assets/1a62e8b2-4a04-4d01-b44e-36dc1a32467e)


### 🚧 Sistema de Colisiones

* Se desarrolló la función `CheckCameraCollision` para evitar que el jugador atraviese muros.
* Se analiza si la cámara está dentro de un radio de colisión (0.2 unidades) respecto a cualquier muro.
* Se aplica una corrección de dirección (normal de colisión) para empujar al jugador fuera del muro en caso de colisión.

### 📚 Referencias

* [Tutorial 22 – Hybrid Rendering (Diligent Engine)](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial22_HybridRendering)
* [Video de avance (YouTube)](https://youtu.be/lC0jSxF-vkE?si=4t-4PwDTEAKh02iw)

---

## 💡 Entrega 2 – Mapa Personalizado y Linterna 

### 🗺️ Creación del Mapa Personalizado
![image](https://github.com/user-attachments/assets/a951a9bf-f99e-4f1d-862e-994a00ff9ac1)

* Se cambió la generación automatica por un **mapa dibujado como imagen**.
* Se desarrolló un **script en Python** que convierte una imagen PNG en una matriz de elementos del juego:

  * Verde → Muros comunes (1)
  * Rojo → Muros con otra textura (2)
  * Azul → Puertas (3)
  * Otros colores → Pasillos (0)

* Esto otorga **control total** sobre la disposición y facilita la iteración del diseño.

### 🔦 Linterna con Ray Tracing

![luz encendida](https://github.com/user-attachments/assets/6dc7432d-4c23-4d88-9967-2bdd1144f2cd)

* Se implementó una linterna dinámica usando **ray tracing desde shaders**.
* Utiliza posición, dirección y ángulo de apertura para generar un cono de luz.
* Calcula sombras suaves y atenuación con base en la distancia y obstáculos.
* Se aplica iluminación difusa cálida y reflejos suaves.

### 👾 Entidad del Backroom
![image](https://github.com/user-attachments/assets/0a60db06-3d49-4da7-a6ab-0b68e0fede6c)

* Se diseñó un modelo de criatura en **Blender** que será usado como amenaza en el juego.
* Aún en proceso de integración.

### 📚 Referencias

* [Tutorial 22 – Hybrid Rendering (Diligent Engine)](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial22_HybridRendering)
* [Video del sistema de linterna (YouTube)](https://youtu.be/JdZTg8ME6Pg)

---

## 📼 Videos de Avance

* 🎥 [Entrega 1 - Colisiones y Laberinto](https://youtu.be/lC0jSxF-vkE?si=4t-4PwDTEAKh02iw)
* 🎥 [Entrega 2 - Iluminación y Linterna](https://youtu.be/JdZTg8ME6Pg)

---

¿Quieres que lo exporte como archivo `.md` o te gustaría añadir capturas de pantalla?
