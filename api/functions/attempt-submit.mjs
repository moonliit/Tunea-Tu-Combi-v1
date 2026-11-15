import { container } from '../container.mjs';
import { ApiError } from '../identity.mjs';
import { respond, route } from '../http.mjs';

// PUT /api/v1/levels/{levelId}/attempts/{attemptId}: the statistics of one run.
// The attempt id comes from the client, so repeating the call is safe:
// 201 the first time, 200 when it was already recorded.
export const handler = route({
    method: 'PUT',
    path: /^\/api\/v1\/levels\/([^/]+)\/attempts\/([0-9a-f-]{36})$/,
    session: 'identity',
    json: { maxLength: 4096 },
    run: async ({ identity, params, body }) => {
        // the level and the attempt are named by the URL, never by the body
        if (
            !body || typeof body !== 'object' || Array.isArray(body) ||
            'levelId' in body || 'attemptId' in body
        ) {
            throw new ApiError(400, 'INVALID_ATTEMPT');
        }
        const leaderboard = await container.leaderboard();
        const result = await leaderboard.submit(
            identity,
            { ...body, levelId: params[0], attemptId: params[1] }
        );
        return respond(result.submission?.idempotent ? 200 : 201, result);
    }
});
