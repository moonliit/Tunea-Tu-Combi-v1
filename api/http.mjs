import { ApiError, readToken, sessionCookie } from './identity.mjs';
import { container } from './container.mjs';

export const respond = (statusCode, body, cookies, headers) => ({
    statusCode,
    headers: {
        'content-type': 'application/json; charset=utf-8',
        'cache-control': 'no-store',
        ...headers
    },
    ...(cookies ? { cookies } : {}),
    body: JSON.stringify(body)
});

const lowerCaseHeaders = (event) => Object.fromEntries(
    Object.entries(event.headers ?? {}).map(([name, value]) => [name.toLowerCase(), value])
);

// a path parameter arrives percent-encoded ('official%3A1'); a broken
// encoding is a path that does not exist
function matchRoute(event, method, path) {
    if (event.requestContext?.http?.method !== method) return null;
    if (typeof path === 'string') return event.rawPath === path ? [] : null;
    const found = event.rawPath.match(path);
    if (!found) return null;
    try {
        return found.slice(1).map(decodeURIComponent);
    } catch {
        return null;
    }
}

// requests that change data must come from our own page; the content type is
// only demanded of a request that carries a body
function guardMutation(headers, { json, binary }) {
    const publicOrigin = container.publicOrigin();
    if (!publicOrigin) throw new Error('PUBLIC_ORIGIN missing');
    if (headers.origin !== publicOrigin || headers['x-combi-request'] !== '1')
        throw new ApiError(403, 'FORBIDDEN_ORIGIN');
    if (binary) {
        if (headers['content-type'] !== binary.mediaType)
            throw new ApiError(415, binary.wrongType);
    } else if (json && !/^application\/json(?:;|$)/i.test(headers['content-type'] ?? '')) {
        throw new ApiError(415, 'JSON_REQUIRED');
    }
}

function parseJson(event, maxLength) {
    try {
        const raw = event.isBase64Encoded
            ? Buffer.from(event.body ?? '', 'base64').toString('utf8')
            : (event.body ?? '');
        if (raw.length > maxLength) throw new Error();
        return JSON.parse(raw);
    } catch {
        throw new ApiError(400, 'INVALID_JSON');
    }
}

function readBinary(event, { maxBytes, tooLarge }) {
    const raw = event.isBase64Encoded
        ? Buffer.from(event.body ?? '', 'base64')
        : Buffer.from(event.body ?? '', 'binary');
    if (raw.length > maxBytes) throw new ApiError(413, tooLarge);
    return raw;
}

// AWS is throttling, failing on its side, or could not be reached: worth
// retrying later. A dropped connection carries a Node code, not an AWS marker.
const NETWORK_CODES = new Set(['ECONNREFUSED', 'ECONNRESET', 'ETIMEDOUT', 'EAI_AGAIN', 'ENOTFOUND', 'EPIPE']);
const transient = (error) =>
    error?.$retryable !== undefined ||
    error?.$metadata?.httpStatusCode >= 500 ||
    error?.name === 'TimeoutError' ||
    NETWORK_CODES.has(error?.code);

// Builds the entry point of one Lambda. The function declares which request it
// serves and what it needs; this adapter does the work every route shares.
//
//   method / path   the only request this Lambda answers (404 otherwise);
//                   a RegExp path hands its decoded capture groups over as params
//   session         'none'     no cookie is read
//                   'token'    the cookie is read, a missing one is allowed
//                   'identity' the cookie must belong to a live session
//   json            { maxLength } parse the body as JSON
//   binary          { mediaType, maxBytes, wrongType, tooLarge } raw body
//   run             does the one job and returns the response
export function route({ method, path, session = 'none', json, binary, run }) {
    return async function handler(event) {
        try {
            const params = matchRoute(event, method, path);
            if (!params) throw new ApiError(404, 'NOT_FOUND');
            const headers = lowerCaseHeaders(event);
            if (method !== 'GET') guardMutation(headers, { json, binary });

            let token = null;
            let identity = null;
            if (session !== 'none') {
                token = readToken(event);
                if (session === 'identity') {
                    identity = (await (await container.identity()).authenticate(token)).identity;
                }
            }

            let body;
            if (json) body = parseJson(event, json.maxLength);
            if (binary) body = readBinary(event, binary);

            return await run({
                event,
                params,
                query: event.queryStringParameters ?? {},
                headers,
                token,
                identity,
                body
            });
        } catch (error) {
            if (error instanceof ApiError) {
                return respond(
                    error.status,
                    { error: error.code },
                    error.status === 401 ? [sessionCookie('', 0)] : undefined
                );
            }
            return transient(error)
                ? respond(503, { error: 'API_UNAVAILABLE' })
                : respond(500, { error: 'INTERNAL_ERROR' });
        }
    };
}
