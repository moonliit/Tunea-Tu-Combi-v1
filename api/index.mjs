import {
    ApiError,
    createIdentityService,
    publicIdentity,
    readToken,
    sessionCookie
} from './identity.mjs';
import { createLeaderboardService } from './leaderboard.mjs';
import { createPublicationService } from './publications.mjs';

export function createHandler(service, publicOrigin, leaderboard = null, publications = null) {
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
            const catalogRead = path === '/api/catalog' && method === 'GET';
            const ownPublications = path === '/api/publications/mine' && method === 'GET';
            const publishRoute = path === '/api/publications' && method === 'POST';
            const levelMatch = method === 'GET'
                ? path.match(/^\/api\/levels\/([0-9a-f-]+)$/)
                : null;
            const retireMatch = method === 'POST'
                ? path.match(/^\/api\/publications\/([0-9a-f-]+)\/retire$/)
                : null;
            const copyMatch = method === 'POST'
                ? path.match(/^\/api\/publications\/([0-9a-f-]+)\/copy$/)
                : null;

            if (
                !sessionRoute && !renameRoute && !leaderboardRead && !leaderboardSubmit &&
                !catalogRead && !ownPublications && !publishRoute &&
                !levelMatch && !retireMatch && !copyMatch
            )
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
            if (catalogRead) {
                if (!publications) throw new Error('Publications unavailable');
                return respond(200, await publications.catalog(
                    event.queryStringParameters?.query,
                    event.queryStringParameters?.cursor
                ));
            }
            if (levelMatch) {
                if (!publications) throw new Error('Publications unavailable');
                return respond(200, await publications.getLevel(levelMatch[1]));
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
            if (ownPublications) {
                if (!publications) throw new Error('Publications unavailable');
                const authenticated = await service.authenticate(token);
                return respond(200, await publications.mine(
                    authenticated.identity,
                    event.queryStringParameters?.cursor
                ));
            }
            let body;
            try {
                const raw = event.isBase64Encoded
                    ? Buffer.from(event.body ?? '', 'base64').toString('utf8')
                    : (event.body ?? '');
                const maximum = publishRoute ? 2 * 1024 * 1024 : 4096;
                if (raw.length > maximum) throw new Error();
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
            if (publishRoute) {
                if (!publications) throw new Error('Publications unavailable');
                return respond(201, await publications.publish(
                    authenticated.identity,
                    body
                ));
            }
            if (retireMatch) {
                if (!publications) throw new Error('Publications unavailable');
                return respond(200, await publications.retire(
                    authenticated.identity,
                    retireMatch[1]
                ));
            }
            if (copyMatch) {
                if (!publications) throw new Error('Publications unavailable');
                return respond(200, await publications.copy(
                    authenticated.identity,
                    copyMatch[1]
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
            if (!process.env.CONTENT_BUCKET) throw new Error('CONTENT_BUCKET missing');
            const { createStore } = await import('./store.mjs');
            const { createObjectStore } = await import('./objects.mjs');
            const store = createStore(process.env.TABLE_NAME);
            const objects = createObjectStore(process.env.CONTENT_BUCKET);
            configuredHandler = createHandler(
                createIdentityService(store),
                process.env.PUBLIC_ORIGIN,
                createLeaderboardService(store),
                createPublicationService(store, objects)
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
