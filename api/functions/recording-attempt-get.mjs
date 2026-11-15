import { container } from '../container.mjs';
import { recordingResponse } from '../recording-response.mjs';
import { route } from '../http.mjs';

// GET /api/v1/levels/{levelId}/attempts/{attemptId}/recording?sha256=: one
// specific run. The hash pins its exact content, so it never changes and it can
// be cached. An hour at most: a retired level deletes its recordings after 24 h.
export const handler = route({
    method: 'GET',
    path: /^\/api\/v1\/levels\/([^/]+)\/attempts\/([0-9a-f-]{36})\/recording$/,
    run: async ({ params, query }) => {
        const recordings = await container.recordings();
        return recordingResponse(
            await recordings.download(params[0], params[1], query.sha256 ?? null),
            'public, max-age=3600'
        );
    }
});
