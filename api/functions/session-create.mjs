import { container } from '../container.mjs';
import { publicIdentity } from '../identity.mjs';
import { respond, route } from '../http.mjs';

// POST /api/v1/session: 201 and a new identity, or 200 when the cookie is alive
export const handler = route({
    method: 'POST',
    path: '/api/v1/session',
    session: 'token',
    run: async ({ token }) => {
        const result = await (await container.identity()).create(token);
        return respond(
            result.cookie ? 201 : 200,
            { identity: publicIdentity(result.identity) },
            result.cookie ? [result.cookie] : undefined
        );
    }
});
