import { container } from '../container.mjs';
import { RECORDING_MEDIA_TYPE } from '../recordings.mjs';
import { route } from '../http.mjs';

// GET /api/v1/levels/{levelId}/attempts/{attemptId}/recording?sha256=: one run,
// gzip exactly as it sits in S3. The hash pins its content, so it never changes
// and it can be cached.
export const handler = route({
    method: 'GET',
    path: /^\/api\/v1\/levels\/([^/]+)\/attempts\/([0-9a-f-]{36})\/recording$/,
    run: async ({ params, query }) => {
        const recordings = await container.recordings();
        const { body, sha256 } = await recordings.download(params[0], params[1], query.sha256 ?? null);
        return {
            statusCode: 200,
            headers: {
                'content-type': RECORDING_MEDIA_TYPE,
                'cache-control': 'public, max-age=3600',
                etag: `"${sha256}"`
            },
            isBase64Encoded: true,
            body: body.toString('base64')
        };
    }
});
