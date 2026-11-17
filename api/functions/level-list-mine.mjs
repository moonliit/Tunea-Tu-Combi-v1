import { container } from '../container.mjs';
import { respond, route } from '../http.mjs';

// GET /api/v1/me/levels?query=&cursor=: the levels the signed-in identity has published
// (query searches by name). Only reads DynamoDB.
export const handler = route({
    method: 'GET',
    path: '/api/v1/me/levels',
    session: 'identity',
    run: async ({ identity, query }) => {
        const publications = await container.publications({ objects: false });
        return respond(200, await publications.mine(identity, query.cursor, query.query));
    }
});
