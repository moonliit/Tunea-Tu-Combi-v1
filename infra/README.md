# Infraestructura en AWS

Deja el juego completo (la web, la API y la base de datos) funcionando en tu
cuenta de AWS. Todo está descrito en [template.yaml](template.yaml).

## Lo que necesitas

- Una cuenta de AWS y el CLI de AWS con sesión iniciada (`aws configure` o
  `aws sso login`).
- Node.js 22 o más.
- Emscripten, para compilar el juego (los comandos de abajo ya lo compilan).

## Levantarla

```bash
just aws-deploy    # compila el juego, crea todo en AWS y lo sube
```

La primera vez tarda 10 minutos aproximadamente.

## Actualizarla

Vuelve a ejecutar el comando de lo que cambiaste. Solo se actualiza lo que
cambió.

| Cambiaste | Ejecuta |
|---|---|
| El backend (`api/`) o `infra/template.yaml` | `just aws-deploy-api` |
| El juego (`src/` o `assets/`) | `just aws-deploy-web` (lo compila y lo sube) |
| Las dos cosas | `just aws-deploy` |

El juego descarga unos 14 MB al abrirse. Las canciones (`assets/audio/clean` y
`custom`) se suben aparte y se bajan cuando se necesitan. Una canción nueva va en
las dos carpetas, con el mismo nombre.

## Borrarla

```bash
just aws-destroy
```

Borra todo: el juego, la API y **los datos de los jugadores**. No se puede
deshacer, así que te pide escribir el nombre del stack para confirmar.
