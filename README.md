# Proyecto Grafica

Proyecto multiplataforma en C++23 configurado para compilar tanto nativamente en Linux/Mac como para WebAssembly (WASM) utilizando Emscripten y Raylib.

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
