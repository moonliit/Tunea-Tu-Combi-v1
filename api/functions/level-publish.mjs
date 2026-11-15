import { container } from '../container.mjs';
import { respond, route } from '../http.mjs';

// POST /api/v1/levels: publish a custom level together with its winning run.
// 201 and where it lives, or 200 when the same request was already applied.
export const handler = route({
    method: 'POST',
    path: '/api/v1/levels',
    session: 'identity',
    json: { maxLength: 2 * 1024 * 1024 },
    run: async ({ identity, body }) => {
        const publications = await container.publications();
        const published = await publications.publish(identity, body);
        return respond(
            published.idempotent ? 200 : 201,
            published,
            undefined,
            { location: `/api/v1/levels/${encodeURIComponent(published.levelId)}` }
        );
    }
});
