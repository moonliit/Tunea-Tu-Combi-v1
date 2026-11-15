import { container } from '../container.mjs';
import { publicIdOf } from '../level-id.mjs';
import { respond, route } from '../http.mjs';

// POST /api/v1/levels/{levelId}/copies: an editable draft of one of your levels.
// Nothing is stored on the server: the draft comes back in the answer.
export const handler = route({
    method: 'POST',
    path: /^\/api\/v1\/levels\/([^/]+)\/copies$/,
    session: 'identity',
    run: async ({ identity, params }) => {
        const publications = await container.publications();
        return respond(200, await publications.copy(identity, publicIdOf(params[0])));
    }
});
