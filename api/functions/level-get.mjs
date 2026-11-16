import { container } from '../container.mjs';
import { publicIdOf } from '../level-id.mjs';
import { respond, route } from '../http.mjs';

// GET /api/v1/levels/{levelId}: the definition of one published level
export const handler = route({
    method: 'GET',
    path: /^\/api\/v1\/levels\/([^/]+)$/,
    run: async ({ params }) => {
        const publications = await container.publications();
        return respond(200, await publications.getLevel(publicIdOf(params[0])));
    }
});
