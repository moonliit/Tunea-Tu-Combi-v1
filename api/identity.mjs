import { createHash, randomBytes, randomUUID } from 'node:crypto';

export const COOKIE = '__Host-combi-session';
export const SESSION_SECONDS = 365 * 24 * 60 * 60;
export class ApiError extends Error {
    constructor(status, code) {
        super(code);
        this.status = status;
        this.code = code;
    }
}

// NFKC, 3-24 characters: Latin letters, digits, spaces and hyphens
// the key ignores case and extra spaces but preserves accents
export function normalizeName(input) {
    if (typeof input !== 'string' || input.length > 256 || /[\p{C}\p{Zl}\p{Zp}]/u.test(input)) {
        throw new ApiError(400, 'INVALID_NAME');
    }
    const name = input
        .normalize('NFKC')
        .replace(/\p{Zs}+/gu, ' ')
        .trim();
    const chars = Array.from(name);
    const alnum = /^[\p{Script=Latin}0-9]$/u;
    if (
        chars.length < 3 ||
        chars.length > 24 ||
        !alnum.test(chars[0]) ||
        !alnum.test(chars.at(-1)) ||
        !chars.every((c) => (alnum.test(c) && /[\p{L}0-9]/u.test(c)) || c === ' ' || c === '-')
    ) {
        throw new ApiError(400, 'INVALID_NAME');
    }
    return { name, nameKey: name.toUpperCase().normalize('NFKC') };
}

export function readToken(event) {
    const headers = Object.fromEntries(
        Object.entries(event.headers ?? {}).map(([k, v]) => [k.toLowerCase(), v])
    );
    const parts = (event.cookies ?? [headers.cookie ?? '']).flatMap((value) => value.split(/;\s*/));
    const matches = parts.filter((part) => part.startsWith(COOKIE + '='));
    if (!matches.length) return null;
    const token = matches[0].slice(COOKIE.length + 1);
    if (matches.length !== 1 || !/^[A-Za-z0-9_-]{43}$/.test(token))
        throw new ApiError(401, 'INVALID_SESSION');
    return token;
}
export const tokenHash = (token) => createHash('sha256').update(token).digest('hex');
export const sessionCookie = (token, age = SESSION_SECONDS) =>
    `${COOKIE}=${token}; Path=/; HttpOnly; Secure; SameSite=Strict; Max-Age=${age}`;
export const publicIdentity = (item) => ({
    id: item.id,
    name: item.name ?? null,
    version: item.version
});

export function createIdentityService(store, clock = () => Math.floor(Date.now() / 1000)) {
    async function authenticate(token) {
        if (!token) throw new ApiError(401, 'NO_SESSION');
        const hash = tokenHash(token);
        const session = await store.get(`SESSION#${hash}`);
        if (!session || session.expiresAt <= clock()) throw new ApiError(401, 'INVALID_SESSION');
        const identity = await store.get(`COMBI#${session.combiId}`);
        if (!identity) throw new ApiError(401, 'INVALID_SESSION');
        return { identity, sessionHash: hash };
    }
    return {
        authenticate,
        async create(token) {
            if (token) return { identity: (await authenticate(token)).identity };
            const secret = randomBytes(32).toString('base64url');
            const identity = { id: randomUUID(), version: 0, createdAt: clock() };
            await store.create(identity, tokenHash(secret), clock() + SESSION_SECONDS);
            return { identity, cookie: sessionCookie(secret) };
        },
        async rename(token, input, expectedId) {
            const normalized = normalizeName(input);
            // retry conflicts using the latest version
            for (let attempt = 0; attempt < 5; attempt++) {
                const { identity, sessionHash } = await authenticate(token);
                if (identity.id !== expectedId) throw new ApiError(409, 'IDENTITY_CHANGED');
                const next = { ...identity, ...normalized, version: identity.version + 1 };
                try {
                    await store.rename(identity, next, sessionHash, clock());
                    return next;
                } catch (error) {
                    if (error.name !== 'TransactionCanceledException') throw error;
                    const reasons = error.CancellationReasons ?? [];
                    if (
                        reasons.some(
                            (r) =>
                                !['None', 'ConditionalCheckFailed', 'TransactionConflict'].includes(
                                    r.Code
                                )
                        )
                    )
                        throw error;
                    const reservation = await store.get(`NAME#${normalized.nameKey}`);
                    if (reservation && reservation.combiId !== identity.id)
                        throw new ApiError(409, 'NAME_TAKEN');
                    await new Promise((resolve) =>
                        setTimeout(resolve, 20 * (attempt + 1) + Math.random() * 30)
                    );
                }
            }
            throw new ApiError(409, 'RETRY_RENAME');
        }
    };
}
