import { container } from '../container.mjs';
import { ApiError, publicIdentity } from '../identity.mjs';
import { respond, route } from '../http.mjs';

// PUT /api/v1/identities/{identityId}/name: reserve a name for that identity.
// The URL says whose name it is; if the cookie belongs to someone else, 409.
export const handler = route({
    method: 'PUT',
    path: /^\/api\/v1\/identities\/([0-9a-f-]{36})\/name$/,
    session: 'identity',
    json: { maxLength: 4096 },
    run: async ({ token, params, body }) => {
        if (!body || Object.keys(body).length !== 1 || typeof body.name !== 'string')
            throw new ApiError(400, 'INVALID_NAME');
        const identity = await (await container.identity())
            .rename(token, body.name, params[0]);
        return respond(200, { identity: publicIdentity(identity) });
    }
});
