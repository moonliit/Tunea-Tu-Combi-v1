import { container } from '../container.mjs';
import { respond, route } from '../http.mjs';

// GET /api/v1/levels?query=&cursor=: a page of published levels.
// Only reads DynamoDB.
export const handler = route({
    method: 'GET',
    path: '/api/v1/levels',
    run: async ({ query }) => {
        const publications = await container.publications({ objects: false });
        return respond(200, await publications.catalog(query.query, query.cursor));
    }
});
