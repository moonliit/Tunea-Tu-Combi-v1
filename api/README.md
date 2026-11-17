# API (AWS Lambda)

Una Lambda por cada llamada HTTP del juego, más una para la limpieza
programada. Están en `functions/`.

```text
api/
├── functions/     # una Lambda por archivo
├── http.mjs       # lo común a las rutas: origen, sesión, cuerpo, errores
├── container.mjs  # arma DynamoDB, S3 y los servicios que usa cada función
├── identity.mjs, leaderboard.mjs, recordings.mjs, publications.mjs
│                  # reglas del dominio, sin HTTP
├── store.mjs      # DynamoDB (una sola tabla)
└── objects.mjs    # S3
```

## Rutas

Todas empiezan con `/api/v1`. Si cambias una ruta, hay que cambiarla también en el
cliente (`src/web/api.js`).

El nivel va en la URL como `{levelId}`, que puede ser:

- `official:1` hasta `official:20`: los niveles del juego (el mismo máximo que
  `MaxOfficialLevelCount` en `src/app/LevelCatalog.hpp`).
- `public:<uuid>`: los que publican los jugadores.

| Función (`functions/<archivo>.handler`) | Solicitud | Respuesta |
|---|---|---|
| `health` | `GET /health` | 200 |
| `session-get` | `GET /session` | 200 con `identity` y `progress.unlockedLevel`, o 401 si no hay sesión |
| `session-create` | `POST /session` | 201 si crea la identidad (y la cookie), 200 si la cookie sigue válida |
| `identity-rename` | `PUT /identities/{identityId}/name` `{name}` | 200, o 409 si la cookie es de otra identidad |
| `leaderboard-get` | `GET /levels/{levelId}/leaderboard` | 200 |
| `attempt-submit` | `PUT /levels/{levelId}/attempts/{attemptId}` | 201 si el intento es nuevo, 200 si ya existía |
| `recording-upload` | `PUT /levels/{levelId}/attempts/{attemptId}/recording` | 201 si es nueva, 200 si ya existía |
| `recording-champion-get` | `GET /levels/{levelId}/leaderboard/champion/recording` | 200, sin caché |
| `recording-attempt-get` | `GET /levels/{levelId}/attempts/{attemptId}/recording?sha256=` | 200, caché de 1 h |
| `level-list` | `GET /levels?query=&cursor=` | 200 |
| `level-get` | `GET /levels/{levelId}` | 200, o 404 |
| `level-publish` | `POST /levels` | 201 con `Location`, 200 si ya estaba aplicado |
| `level-list-mine` | `GET /me/levels?status=&query=&cursor=` | 200 |
| `level-retire` | `DELETE /levels/{levelId}` | 200 |
| `level-copy` | `POST /levels/{levelId}/copies` | 200, con el borrador en la respuesta |
| `recordings-cleanup` | EventBridge, no es HTTP | - |

- Las solicitudes que escriben piden `Origin` igual a `PUBLIC_ORIGIN` y la
  cabecera `x-combi-request: 1`. El `content-type` solo se pide si hay cuerpo.
- Errores: `{ "error": "CODIGO" }`. 4xx si falla quien llama, 500
  `INTERNAL_ERROR` si falla el código, 503 `API_UNAVAILABLE` si AWS limita o no
  responde (se puede reintentar).
- `level-get`, `level-retire` y `level-copy` solo aceptan niveles públicos. Con
  un `official:<n>` responden 404 `LEVEL_NOT_FOUND`.
- `levelId` y `attemptId` van en la URL, no en el cuerpo. `attempt-submit`
  rechaza un cuerpo que los traiga.
- Las grabaciones por hash no cambian, así que se cachean 1 h. No más, porque al
  retirar un nivel se borran a las 24 h.

## Variables y permisos

| Función | `TABLE_NAME` | `CONTENT_BUCKET` | `PUBLIC_ORIGIN` | DynamoDB | S3 |
|---|:-:|:-:|:-:|---|---|
| `health` | | | | - | - |
| `session-get` | x | | | lectura | - |
| `session-create` | x | | x | lectura y escritura transaccional | - |
| `identity-rename` | x | | x | lectura y escritura transaccional | - |
| `leaderboard-get` | x | | | lectura | - |
| `attempt-submit` | x | x | x | lectura y escritura (también transaccional) | `PutObjectTagging` |
| `recording-upload` | x | x | x | lectura y escritura | `PutObject`, `PutObjectTagging` |
| `recording-champion-get` | x | x | | lectura | `GetObject`, `ListBucket` |
| `recording-attempt-get` | x | x | | lectura | `GetObject`, `ListBucket` |
| `level-list` | x | | | lectura | - |
| `level-get` | x | x | | lectura | `GetObject`, `ListBucket` |
| `level-publish` | x | x | x | lectura y escritura transaccional | `PutObject`, `PutObjectTagging`, `DeleteObject` |
| `level-list-mine` | x | | | lectura | - |
| `level-retire` | x | x | x | lectura y escritura transaccional | `PutObjectTagging` |
| `level-copy` | x | x | x | lectura | `GetObject`, `ListBucket` |
| `recordings-cleanup` | x | x | | lectura | `ListBucket`, `GetObjectTagging`, `PutObjectTagging`, `DeleteObject` |
