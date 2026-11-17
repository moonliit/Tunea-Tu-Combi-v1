# Proyecto Grafica

Proyecto multiplataforma en C++23 configurado para compilar tanto nativamente en Linux/Mac como para WebAssembly (WASM) utilizando Emscripten y Raylib.

## Requisitos Previos

### 1. Desarrollo Nativo (Linux y macOS)

- **Compilador C++23**: `g++` (o `clang++`)
- **Librerías del sistema**: `raylib`
- **Automatización**: `just`

En Arch Linux por ejemplo puedes instalar los paquetes base con:

```bash
sudo pacman -S gcc cmake just raylib
```

En macOS con Homebrew:

```bash
brew install just cmake raylib
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
| `just web`   | Compila la versión WebAssembly (Debug) en `build/web/`                         |
| `just web-release` | Compila la versión WebAssembly optimizada en `build/web-release/`        |
| `just serve` | Compila para WebAssembly y levanta un servidor local (`http://localhost:6931`) |
| `just aws-deploy` | Compila el juego (Release) y crea o actualiza todo en AWS: infraestructura, Lambdas y web (ver `infra/README.md`) |
| `just aws-deploy-api` | Crea o actualiza solo la infraestructura y las Lambdas de AWS               |
| `just aws-deploy-web` | Compila el juego (Release) y lo sube al sitio desplegado                    |
| `just aws-outputs` | Muestra dónde quedó todo en AWS, como la dirección del juego                   |
| `just aws-destroy` | Borra todo en AWS, incluidos los datos de los jugadores (pide confirmación)    |
| `just clean` | Elimina los artefactos compilados                                              |

Al jugar con `just run` desde la raíz del proyecto, los niveles oficiales
desbloqueados se guardan al completar cada nivel en
`user_data/official_progress.json`. El archivo persiste entre ejecuciones y
está excluido de Git. Los récords personales y los niveles personalizados
se guardan por separado en la misma carpeta.

## Estructura del Proyecto

```text
.
├── api/             # Backend en AWS Lambda
├── assets/          # Texturas, fuentes, sonidos y recursos del juego
├── infra/           # Plantilla de AWS y script para desplegar todo desde cero
├── build/           # Artefactos compilados (native/ y web/)
├── include/         # Cabeceras de terceros y propias
│   ├── raylib/
│   └── raylib-cpp/
├── lib/
│   └── web/         # Raylib compilado estáticamente para WASM
├── src/             # Código fuente C++ (.cpp)
├── Justfile         # Recetas de compilación
└── README.md
```

## Texturas

Están en `assets/textures/` y se cargan una sola vez al iniciar el juego, en `TextureLibrary`.

Las de las piezas (`parts/`) son provisionales, hechas con formas simples. Para cambiarlas basta con reemplazar el PNG dejando el mismo nombre. Cada una es un cuadrado que se dibuja centrado en la celda (la rueda usa su diámetro). Sin rotar, el ventilador y la vela empujan hacia la derecha y el chofer mira hacia la derecha. `wheel_mount.png` es el soporte de la rueda y el centro de la imagen es el punto donde se une a la celda. La caja tiene que ser translúcida por dentro porque ahí van el chofer y el TNT.

El fondo (`backgrounds/`) no se mueve con la cámara y es fijo para cada nivel: el 1 usa Miraflores, el 2 Rímac, el 3 Chorrillos y el 4 Barranco. Los niveles personalizados usan Miraflores. La relación nivel-fondo está en `GetLevelBackground`, en `src/rendering/Background.cpp`. Los fondos son PNG de 800x600.

## Música

Las versiones editadas se prepararon en Audacity con la librería Bit Crusher y compresión de sonido.

## Grabaciones del campeón

Se muestrean a 20 Hz, con límites de 2 MiB gzip y 32 MiB descomprimidos. Cada
partida ganada tiene un id propio (`attemptId`) y cada grabación tiene un hash
(`sha256`), un código calculado con el contenido del archivo. Para mostrar al
campeón, el juego toma esos dos datos del ranking y pide la grabación con ellos.

## Referencias de arquitectura
- Lambda con Node.js y módulos `.mjs`:  https://docs.aws.amazon.com/lambda/latest/dg/nodejs-handler.html
- API Gateway HTTP API con integración Lambda proxy:  https://docs.aws.amazon.com/apigateway/latest/developerguide/http-api-develop-integrations-lambda.html
- S3 privado como origen de CloudFront mediante OAC: https://docs.aws.amazon.com/AmazonCloudFront/latest/DeveloperGuide/private-content-restricting-access-to-s3.html
- Transacciones y escrituras condicionales en DynamoDB: https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/transaction-apis.html
