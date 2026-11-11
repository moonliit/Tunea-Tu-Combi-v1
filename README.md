# Proyecto Grafica

Proyecto multiplataforma en C++23 configurado para compilar tanto nativamente en Linux como para WebAssembly (WASM) utilizando Emscripten y Raylib.

## Requisitos Previos

### 1. Desarrollo Nativo (Linux y macOS)

- **Compilador C++23**: `g++` (o `clang++`)
- **Librerías del sistema**: `raylib`
- **Automatización**: `just`

En Arch Linux por ejemplo puedes instalar los paquetes base con:

```bash
sudo pacman -S gcc just raylib
```

En macOS con Homebrew:

```bash
brew install just cmake raylib box2d
```

---

### 2. Desarrollo Web (WebAssembly)

- **Emscripten SDK (`emsdk`)**: Proporciona `em++` y `emrun`.

Para instalarlo o activarlo:

```bash
git clone [https://github.com/emscripten-core/emsdk.git](https://github.com/emscripten-core/emsdk.git)
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

---

## Uso del Proyecto (`Justfile` y CMake Presets)

El proyecto utiliza `just` como wrapper de CMake. Los presets usan explícitamente
`Unix Makefiles`, habilitan diagnósticos coloreados y conservan compilación
incremental. CMake genera `compile_commands.json` dentro del build nativo y
`just build` crea un symlink en la raíz para herramientas LSP.

| Comando      | Descripción                                                                    |
| ------------ | ------------------------------------------------------------------------------ |
| `just build` | Compila el ejecutable nativo en `build/native/`                                |
| `just run`   | Compila y ejecuta la versión nativa                                            |
| `just web`   | Compila la versión WebAssembly en `build/web/`                                 |
| `just serve` | Compila para WebAssembly y levanta un servidor local (`http://localhost:6931`) |
| `just clean` | Elimina los artefactos compilados                                              |

## Estructura del Proyecto

```text
.
├── assets/          # Texturas, fuentes, sonidos y recursos del juego
├── build/           # Artefactos compilados (native/ y web/)
├── include/         # Cabeceras de terceros y propias
│   ├── raylib/
│   └── raylib-cpp/
├── lib/
│   ├── native/      # Box2D compilado estáticamente para Linux
│   └── web/         # Raylib y Box2D compilados estáticamente para WASM
├── src/             # Código fuente C++ (.cpp)
├── Justfile         # Recetas de compilación
└── README.md
```

## Texturas

Están en `assets/textures/` y se cargan una sola vez al iniciar el juego, en `TextureLibrary`.

Las de las piezas (`parts/`) son provisionales, hechas con formas simples. Para cambiarlas basta con reemplazar el PNG dejando el mismo nombre. Cada una es un cuadrado que se dibuja centrado en la celda (la rueda usa su diámetro). Sin rotar, el ventilador y la vela empujan hacia la derecha y el chofer mira hacia la derecha. `wheel_mount.png` es el soporte de la rueda y el centro de la imagen es el punto donde se une a la celda. La caja tiene que ser translúcida por dentro porque ahí van el chofer y el TNT.

El fondo (`backgrounds/`) no se mueve con la cámara y es fijo para cada nivel: el 1 usa Miraflores, el 2 Rímac, el 3 Chorrillos y el 4 Barranco. Los niveles personalizados usan Miraflores. La relación nivel-fondo está en `GetLevelBackground`, en `src/rendering/Background.cpp`.

Los fondos son PNG de 800x600. No sirve JPG porque el raylib que usamos para web no lo decodifica. Mejor usar fotos con mucho cielo o mar y poco detalle en el centro, porque si tienen casas o gente se confunden con el juego.
