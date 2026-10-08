# Proyecto Grafica

Proyecto multiplataforma en C++23 configurado para compilar tanto nativamente en Linux como para WebAssembly (WASM) utilizando Emscripten y Raylib.

## Requisitos Previos

### 1. Desarrollo Nativo (Linux)

- **Compilador C++23**: `g++` (o `clang++`)
- **Librerías del sistema**: `raylib`
- **Automatización**: `just`

En Arch Linux por ejemplo puedes instalar los paquetes base con:

```bash
sudo pacman -S gcc just raylib
```

> **Nota para LSP (Neovim/VSCode/clangd):** Opcionalmente puedes instalar `bear` si deseas generar `compile_commands.json`.

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

## Uso del Proyecto (`Justfile`)

El proyecto utiliza `just` para gestionar los comandos de compilación.

| Comando       | Descripción                                                                    |
| ------------- | ------------------------------------------------------------------------------ |
| `just build`  | Compila el ejecutable nativo en `build/native/`                                |
| `just run`    | Compila y ejecuta la versión nativa                                            |
| `just web`    | Compila la versión WebAssembly en `build/web/`                                 |
| `just serve`  | Compila para WebAssembly y levanta un servidor local (`http://localhost:6931`) |
| `just compdb` | Genera `compile_commands.json` usando `bear` _(Opcional)_                      |
| `just clean`  | Elimina los artefactos de compilación (`build/`, `obj/`, `dep/`)               |

---

## Estructura del Proyecto

```text
.
├── assets/          # Texturas, fuentes, sonidos y recursos del juego
├── build/           # Artefactos compilados (native/ y web/)
├── include/         # Cabeceras de terceros y propias
│   ├── raylib/
│   └── raylib-cpp/
├── lib/
│   └── web/         # Raylib compilado estáticamente para WASM (libraylib.web.a)
├── src/             # Código fuente C++ (.cpp)
├── Justfile         # Recetas de compilación
└── README.md
```
