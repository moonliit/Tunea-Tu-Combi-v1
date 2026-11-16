import { container } from '../container.mjs';
import { publicIdentity } from '../identity.mjs';
import { respond, route } from '../http.mjs';

// GET /api/v1/session: who is signed in with this cookie, and how far they got
export const handler = route({
    method: 'GET',
    path: '/api/v1/session',
    session: 'identity',
    run: async ({ identity }) => respond(200, {
        identity: publicIdentity(identity),
        progress: await (await container.progress()).get(identity.id)
    })
});
