import { container } from '../container.mjs';
import { MAX_COMPRESSED_BYTES, RECORDING_MEDIA_TYPE } from '../recordings.mjs';
import { respond, route } from '../http.mjs';

// PUT /api/v1/levels/{levelId}/attempts/{attemptId}/recording: the gzip run of
// the current champion. 201 the first time, 200 when it was already stored.
export const handler = route({
    method: 'PUT',
    path: /^\/api\/v1\/levels\/([^/]+)\/attempts\/([0-9a-f-]{36})\/recording$/,
    session: 'identity',
    binary: {
        mediaType: RECORDING_MEDIA_TYPE,
        maxBytes: MAX_COMPRESSED_BYTES,
        wrongType: 'RECORDING_GZIP_REQUIRED',
        tooLarge: 'RECORDING_TOO_LARGE'
    },
    run: async ({ identity, params, headers, body }) => {
        const recordings = await container.recordings();
        const result = await recordings.upload(
            identity, params[0], params[1], body, headers['content-type']
        );
        return respond(result.idempotent ? 200 : 201, result);
    }
});
