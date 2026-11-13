import {
    ApiError,
    createIdentityService,
    publicIdentity,
    readToken,
    sessionCookie
} from './identity.mjs';
import { createLeaderboardService } from './leaderboard.mjs';

export function createHandler(service, publicOrigin, leaderboard = null) {
    return async (event) => {
        const respond = (statusCode, body, cookies) => ({
            statusCode,
            headers: {
                'content-type': 'application/json; charset=utf-8',
                'cache-control': 'no-store'
            },
            ...(cookies ? { cookies } : {}),
            body: JSON.stringify(body)
        });
        try {
            const method = event.requestContext?.http?.method;
            const path = event.rawPath;
            if (method === 'GET' && path === '/api/health')
                return respond(200, { ok: true, service: 'tunea-tu-combi' });
            const sessionRoute = path === '/api/session' && ['GET', 'POST'].includes(method);
            const renameRoute = path === '/api/identity/name' && method === 'PUT';
            const leaderboardRead = path === '/api/leaderboard' && method === 'GET';
            const leaderboardSubmit =
                path === '/api/leaderboard/attempts' && method === 'POST';

            if (!sessionRoute && !renameRoute && !leaderboardRead && !leaderboardSubmit)
                throw new ApiError(404, 'NOT_FOUND');
            if (method !== 'GET') {
                const headers = Object.fromEntries(
                    Object.entries(event.headers ?? {}).map(([k, v]) => [k.toLowerCase(), v])
                );
                // exact CloudFront HTTPS origin
                if (!publicOrigin) throw new Error('PUBLIC_ORIGIN missing');
                if (headers.origin !== publicOrigin || headers['x-combi-request'] !== '1')
                    throw new ApiError(403, 'FORBIDDEN_ORIGIN');
                if (!/^application\/json(?:;|$)/i.test(headers['content-type'] ?? ''))
                    throw new ApiError(415, 'JSON_REQUIRED');
            }
            if (leaderboardRead) {
                if (!leaderboard) throw new Error('Leaderboard unavailable');
                return respond(200, await leaderboard.get(
                    event.queryStringParameters?.levelId
                ));
            }
            const token = readToken(event);
            if (sessionRoute && method === 'GET') {
                return respond(200, {
                    identity: publicIdentity((await service.authenticate(token)).identity)
                });
            }
            if (sessionRoute) {
                const result = await service.create(token);
                return respond(
                    result.cookie ? 201 : 200,
                    { identity: publicIdentity(result.identity) },
                    result.cookie ? [result.cookie] : undefined
                );
            }
            let body;
            try {
                const raw = event.isBase64Encoded
                    ? Buffer.from(event.body ?? '', 'base64').toString('utf8')
                    : (event.body ?? '');
                if (raw.length > 4096) throw new Error();
                body = JSON.parse(raw);
            } catch {
                throw new ApiError(400, 'INVALID_JSON');
            }
            const authenticated = await service.authenticate(token);
            if (leaderboardSubmit) {
                if (!leaderboard) throw new Error('Leaderboard unavailable');
                return respond(200, await leaderboard.submit(
                    authenticated.identity,
                    body
                ));
            }
            if (
                !body ||
                Object.keys(body).length !== 2 ||
                typeof body.name !== 'string' ||
                typeof body.expectedId !== 'string'
            )
                throw new ApiError(400, 'INVALID_NAME');
            return respond(200, {
                identity: publicIdentity(await service.rename(token, body.name, body.expectedId))
            });
        } catch (error) {
            if (error instanceof ApiError)
                return respond(
                    error.status,
                    { error: error.code },
                    error.status === 401 ? [sessionCookie('', 0)] : undefined
                );
            return respond(503, { error: 'API_UNAVAILABLE' });
        }
    };
}

let configuredHandler;
export async function handler(event) {
    if (event.rawPath === '/api/health' && event.requestContext?.http?.method === 'GET') {
        return createHandler(null, null)(event);
    }
    try {
        if (!configuredHandler) {
            if (!process.env.TABLE_NAME) throw new Error('TABLE_NAME missing');
            const { createStore } = await import('./store.mjs');
            const store = createStore(process.env.TABLE_NAME);
            configuredHandler = createHandler(
                createIdentityService(store),
                process.env.PUBLIC_ORIGIN,
                createLeaderboardService(store)
            );
        }
        return await configuredHandler(event);
    } catch {
        return {
            statusCode: 503,
            headers: { 'content-type': 'application/json', 'cache-control': 'no-store' },
            body: '{"error":"API_UNAVAILABLE"}'
        };
    }
}
