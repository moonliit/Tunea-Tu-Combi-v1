import { container } from '../container.mjs';
import { publicIdOf } from '../level-id.mjs';
import { respond, route } from '../http.mjs';

// DELETE /api/v1/levels/{levelId}: take one of your levels out of the catalog.
// It cannot be published again; its scores stay.
export const handler = route({
    method: 'DELETE',
    path: /^\/api\/v1\/levels\/([^/]+)$/,
    session: 'identity',
    run: async ({ identity, params }) => {
        const publications = await container.publications({ objects: false });
        return respond(200, await publications.retire(identity, publicIdOf(params[0])));
    }
});
