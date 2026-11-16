import { container } from '../container.mjs';
import { respond, route } from '../http.mjs';

// GET /api/v1/levels/{levelId}/leaderboard: top scores of a level.
// Only reads DynamoDB.
export const handler = route({
    method: 'GET',
    path: /^\/api\/v1\/levels\/([^/]+)\/leaderboard$/,
    run: async ({ params }) => {
        const leaderboard = await container.leaderboard({ objects: false });
        return respond(200, await leaderboard.get(params[0]));
    }
});
