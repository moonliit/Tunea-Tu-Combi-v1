import { createHash } from 'node:crypto';
import { ApiError } from './identity.mjs';
import {
    CAPTURE_FORMAT_VERSION,
    PHYSICS_VERSION,
    RULES_VERSION,
    attemptFingerprint,
    isObject
} from './leaderboard.mjs';
import {
    ORPHAN_GRACE_MS,
    SAMPLE_INTERVAL_MS,
    encodeRecording
} from './recordings.mjs';

const MAX_ACTIVE_PUBLICATIONS = 5;
const CATALOG_PAGE_SIZE = 10;
const SEARCH_SCOPE = 100;
const UUID = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const PUBLIC_ID = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-8][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

const exactKeys = (value, keys) =>
    value &&
    typeof value === 'object' &&
    !Array.isArray(value) &&
    JSON.stringify(Object.keys(value).sort()) === JSON.stringify([...keys].sort());

function canonical(value) {
    if (Array.isArray(value)) return value.map(canonical);
    if (value && typeof value === 'object') {
        return Object.fromEntries(
            Object.keys(value).sort().map((key) => [key, canonical(value[key])])
        );
    }
    return Object.is(value, -0) ? 0 : value;
}

const canonicalText = (value) => JSON.stringify(canonical(value));
const hashValue = (value) => createHash('sha256').update(canonicalText(value)).digest('hex');

function stableLevelId(authorId, requestId) {
    const bytes = createHash('sha256').update(`${authorId}\0${requestId}`).digest().subarray(0, 16);
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;
    const hex = bytes.toString('hex');
    return `${hex.slice(0, 8)}-${hex.slice(8, 12)}-${hex.slice(12, 16)}-${hex.slice(16, 20)}-${hex.slice(20)}`;
}

// The game is trusted for the content of a publication.
// The server only reads what it needs to store it and to rank it.
function validatePublishBody(body) {
    const { capture, definition, draftId, music, name, publicationRequestId } = body ?? {};
    // these ids end up in storage keys, so they must at least be well formed
    if (
        !capture || !definition || !music || typeof name !== 'string' ||
        !UUID.test(publicationRequestId) || !UUID.test(draftId) || !UUID.test(capture.attemptId) ||
        !isObject(capture.statistics) || !isObject(capture.score)
    ) {
        throw new ApiError(400, 'INVALID_PUBLICATION');
    }
    if (
        capture.formatVersion !== CAPTURE_FORMAT_VERSION ||
        capture.physicsVersion !== PHYSICS_VERSION ||
        capture.rulesVersion !== RULES_VERSION
    ) {
        throw new ApiError(409, 'CAPTURE_VERSION_MISMATCH');
    }
    return {
        publicationRequestId,
        draftId,
        name,
        music,
        definition,
        capture: canonical(capture),
        score: capture.score
    };
}

const publicationResponse = (level, idempotent = false) => ({
    id: level.id,
    levelId: `public:${level.id}`,
    name: level.name,
    status: level.visibility,
    publishedAt: level.publishedAt,
    verification: 'recorded-trace-only',
    idempotent
});

function publicId(value) {
    if (typeof value !== 'string' || !PUBLIC_ID.test(value))
        throw new ApiError(400, 'INVALID_LEVEL');
    return value;
}

function normalizeQuery(value) {
    if (value === undefined || value === '') return '';
    if (typeof value !== 'string' || value.length > 160)
        throw new ApiError(400, 'INVALID_SEARCH');
    const query = value.normalize('NFKC').replace(/\p{Zs}+/gu, ' ').trim().toLocaleLowerCase('es');
    if (Array.from(query).length > 40 || /[\p{C}]/u.test(query))
        throw new ApiError(400, 'INVALID_SEARCH');
    return query;
}

function readCatalogCursor(value, query) {
    if (!value) return { store: null, remaining: query ? SEARCH_SCOPE : null };
    try {
        const parsed = JSON.parse(Buffer.from(value, 'base64url').toString('utf8'));
        if (
            !exactKeys(parsed, ['query', 'remaining', 'store']) ||
            parsed.query !== query ||
            (parsed.store !== null && typeof parsed.store !== 'string') ||
            (query && (!Number.isSafeInteger(parsed.remaining) || parsed.remaining < 1 || parsed.remaining > SEARCH_SCOPE)) ||
            (!query && parsed.remaining !== null)
        ) {
            throw new Error();
        }
        return parsed;
    } catch {
        throw new ApiError(400, 'INVALID_CURSOR');
    }
}

const catalogCursor = (storeCursor, query, remaining) => storeCursor
    ? Buffer.from(JSON.stringify({ store: storeCursor, query, remaining })).toString('base64url')
    : null;

export function createPublicationService(store, objects, clock = () => Date.now()) {
    async function readPackage(level) {
        const value = await objects.getJson(level.levelKey);
        if (
            !exactKeys(value, [
                'definition', 'formatVersion', 'id', 'levelFingerprint', 'music', 'name'
            ]) ||
            value.formatVersion !== 'level-v1' ||
            value.id !== level.id ||
            value.name !== level.name
        ) {
            throw new Error('Invalid level object');
        }
        if (
            value.levelFingerprint !== level.levelFingerprint ||
            hashValue(value.definition) !== level.levelFingerprint
        ) {
            throw new Error('Level fingerprint mismatch');
        }
        return { definition: value.definition, music: value.music };
    }

    async function rowsFor(items) {
        const identities = await store.getIdentities(items.map((item) => item.authorId));
        const boards = await store.getLeaderboards(items.map((item) => `public:${item.levelId}`));
        return items.map((item) => {
            const champion = boards.get(`public:${item.levelId}`)?.entries?.[0] ?? null;
            if (!champion) throw new Error('Published level without first score');
            return {
                id: item.levelId,
                name: item.name,
                author: {
                    id: item.authorId,
                    name: identities.get(item.authorId)?.name ?? null
                },
                publishedAt: item.publishedAt,
                best: {
                    total: champion.score.total,
                    driverTimeMs: champion.statistics.driverTimeMs,
                    initialParts: champion.statistics.initialParts
                }
            };
        });
    }

    return {
        async publish(identity, body) {
            const parsed = validatePublishBody(body);
            const payloadFingerprint = hashValue(parsed);
            const previousReceipt = await store.getPublicationReceipt(
                identity.id,
                parsed.publicationRequestId
            );
            if (previousReceipt) {
                if (previousReceipt.payloadFingerprint !== payloadFingerprint)
                    throw new ApiError(409, 'PUBLICATION_REQUEST_CONFLICT');
                const level = await store.getLevel(previousReceipt.levelId);
                return publicationResponse(level, true);
            }
            const previousAttempt = await store.getPublishedAttempt(
                identity.id,
                parsed.capture.attemptId
            );
            if (previousAttempt) throw new ApiError(409, 'ATTEMPT_ALREADY_PUBLISHED');
            if (await store.getPublishedDraft(identity.id, parsed.draftId))
                throw new ApiError(409, 'DRAFT_ALREADY_PUBLISHED');

            const id = stableLevelId(identity.id, parsed.publicationRequestId);
            const publishedAt = clock();
            const levelFingerprint = hashValue(parsed.definition);
            const levelKey = `levels/${id}/${payloadFingerprint}.json`;
            // The author won an unpublished draft (levelId == ""). Once the
            // transaction assigns its immutable public ID, the replay must carry
            // that ID or a compatible client will correctly reject it.
            const publishedCapture = {
                ...parsed.capture,
                levelId: `public:${id}`
            };
            const encodedRun = encodeRecording(publishedCapture);
            const runKey = `recordings/public%3A${id}/${parsed.capture.attemptId}-${encodedRun.sha256}.json.gz`;
            const recording = {
                key: runKey,
                attemptId: parsed.capture.attemptId,
                combiId: identity.id,
                levelId: `public:${id}`,
                formatVersion: CAPTURE_FORMAT_VERSION,
                physicsVersion: PHYSICS_VERSION,
                rulesVersion: RULES_VERSION,
                encoding: 'gzip',
                sampleIntervalMs: SAMPLE_INTERVAL_MS,
                sha256: encodedRun.sha256,
                compressedBytes: encodedRun.compressedBytes,
                uncompressedBytes: encodedRun.uncompressedBytes,
                durationMs: parsed.capture.finishedAtMs,
                createdAt: publishedAt
            };
            const packageValue = {
                formatVersion: 'level-v1',
                id,
                name: parsed.name,
                definition: parsed.definition,
                music: parsed.music,
                levelFingerprint
            };
            const leaderboardBody = {
                attemptId: parsed.capture.attemptId,
                formatVersion: CAPTURE_FORMAT_VERSION,
                levelId: `public:${id}`,
                physicsVersion: PHYSICS_VERSION,
                rulesVersion: RULES_VERSION,
                statistics: parsed.capture.statistics
            };
            const firstEntry = {
                combiId: identity.id,
                attemptId: parsed.capture.attemptId,
                runRef: runKey,
                submittedAt: publishedAt,
                statistics: parsed.capture.statistics,
                score: parsed.score,
                levelFingerprint
            };
            const board = {
                revision: 1,
                entries: [firstEntry],
                championRecording: recording,
                receipts: [{
                    attemptId: parsed.capture.attemptId,
                    combiId: identity.id,
                    fingerprint: attemptFingerprint(identity.id, leaderboardBody),
                    receivedAt: publishedAt
                }],
                updatedAt: publishedAt
            };
            const catalogSk = `${String(publishedAt).padStart(13, '0')}#${id}`;
            const authorSk = `LEVEL#${String(publishedAt).padStart(13, '0')}#${id}`;
            const level = {
                id,
                authorId: identity.id,
                draftId: parsed.draftId,
                name: parsed.name,
                visibility: 'public',
                publishedAt,
                retiredAt: null,
                levelFingerprint,
                levelKey,
                runKey,
                firstAttemptId: parsed.capture.attemptId,
                catalogSk,
                authorSk,
                verification: 'recorded-trace-only'
            };

            try {
                await objects.putJson(levelKey, packageValue);
                await objects.putRecording(runKey, encodedRun.body, recording);
                await store.publishLevel({
                    level,
                    board,
                    payloadFingerprint,
                    publicationRequestId: parsed.publicationRequestId
                });
                await objects.markRecordingCurrent(runKey).catch(() => {});
                return publicationResponse(level);
            } catch (error) {
                const committed = await store.getPublicationReceipt(
                    identity.id,
                    parsed.publicationRequestId
                );
                if (committed) {
                    if (committed.payloadFingerprint !== payloadFingerprint)
                        throw new ApiError(409, 'PUBLICATION_REQUEST_CONFLICT');
                    return publicationResponse(await store.getLevel(committed.levelId), true);
                }
                await objects.deleteMany([levelKey, runKey]).catch(() => {});
                if (error instanceof ApiError) throw error;
                if (error?.name === 'TransactionCanceledException') {
                    if (await store.getPublishedAttempt(identity.id, parsed.capture.attemptId))
                        throw new ApiError(409, 'ATTEMPT_ALREADY_PUBLISHED');
                    if (await store.getPublishedDraft(identity.id, parsed.draftId))
                        throw new ApiError(409, 'DRAFT_ALREADY_PUBLISHED');
                    if ((await store.getAuthorState(identity.id))?.activePublications >= MAX_ACTIVE_PUBLICATIONS)
                        throw new ApiError(409, 'PUBLICATION_LIMIT');
                    throw new ApiError(409, 'RETRY_PUBLICATION');
                }
                throw error;
            }
        },

        async catalog(queryValue, cursor) {
            const query = normalizeQuery(queryValue);
            const cursorState = readCatalogCursor(cursor, query);
            const matches = [];
            let nextStoreCursor = cursorState.store;
            let inspected = 0;
            do {
                let page;
                try {
                    page = await store.queryCatalog(
                        nextStoreCursor,
                        query
                            ? Math.min(CATALOG_PAGE_SIZE, cursorState.remaining - inspected)
                            : CATALOG_PAGE_SIZE
                    );
                } catch (error) {
                    if (error?.message === 'Invalid cursor')
                        throw new ApiError(400, 'INVALID_CURSOR');
                    throw error;
                }
                nextStoreCursor = page.cursor;
                inspected += page.items.length;
                const identities = await store.getIdentities(page.items.map((item) => item.authorId));
                for (const item of page.items) {
                    const author = identities.get(item.authorId)?.name ?? '';
                    const searchText = `${item.name} ${author}`.normalize('NFKC').toLocaleLowerCase('es');
                    if (!query || searchText.includes(query)) matches.push(item);
                }
            } while (
                query &&
                matches.length < CATALOG_PAGE_SIZE &&
                nextStoreCursor &&
                inspected < cursorState.remaining
            );
            const remaining = query
                ? Math.max(0, cursorState.remaining - inspected)
                : null;
            return {
                items: await rowsFor(matches.slice(0, CATALOG_PAGE_SIZE)),
                cursor: remaining === 0
                    ? null
                    : catalogCursor(nextStoreCursor, query, remaining),
                searchScope: query ? 'latest-100' : 'all-recent-first',
                inspected
            };
        },

        // status: 'public' or 'retired' to list only those (omitted lists both).
        // query: part of the name, normalized like the catalog search.
        async mine(identity, cursor, status, queryValue) {
            if (status !== undefined && !['public', 'retired'].includes(status))
                throw new ApiError(400, 'INVALID_STATUS');
            const query = normalizeQuery(queryValue);
            let page;
            let items;
            do {
                try {
                    page = await store.queryAuthorLevels(identity.id, cursor, 20, status);
                } catch (error) {
                    if (error?.message === 'Invalid cursor')
                        throw new ApiError(400, 'INVALID_CURSOR');
                    throw error;
                }
                cursor = page.cursor;
                items = page.items.filter((item) =>
                    item.name.normalize('NFKC').toLocaleLowerCase('es').includes(query));
                // A filtered page can be empty although more levels follow: keep reading.
            } while (!items.length && cursor);
            return {
                activeCount: (await store.getAuthorState(identity.id))?.activePublications ?? 0,
                limit: MAX_ACTIVE_PUBLICATIONS,
                items: items.map((item) => ({
                    id: item.levelId,
                    name: item.name,
                    status: item.visibility,
                    publishedAt: item.publishedAt,
                    retiredAt: item.retiredAt ?? null
                })),
                cursor
            };
        },

        async getLevel(idValue) {
            const id = publicId(idValue);
            const level = await store.getLevel(id);
            if (!level || level.visibility !== 'public')
                throw new ApiError(404, 'LEVEL_NOT_FOUND');
            const packageValue = await readPackage(level);
            return {
                id,
                name: level.name,
                definition: { ...packageValue.definition, publicId: id },
                music: packageValue.music
            };
        },

        async retire(identity, idValue) {
            const id = publicId(idValue);
            const level = await store.getLevel(id);
            if (!level) throw new ApiError(404, 'LEVEL_NOT_FOUND');
            if (level.authorId !== identity.id) throw new ApiError(403, 'NOT_LEVEL_AUTHOR');
            if (level.visibility !== 'public') throw new ApiError(409, 'LEVEL_ALREADY_RETIRED');
            const retiredAt = clock();
            try {
                await store.retireLevel(level, retiredAt);
            } catch (error) {
                if (error?.name === 'TransactionCanceledException') {
                    const current = await store.getLevel(id);
                    if (current?.visibility === 'retired')
                        throw new ApiError(409, 'LEVEL_ALREADY_RETIRED');
                }
                throw error;
            }
            const board = await store.getLeaderboard(`public:${id}`);
            if (board?.championRecording?.key) {
                await objects.markRecordingOrphan(
                    board.championRecording.key,
                    retiredAt + ORPHAN_GRACE_MS
                ).catch(() => {});
            }
            return { id, status: 'retired', retiredAt };
        },

        async copy(identity, idValue) {
            const id = publicId(idValue);
            const level = await store.getLevel(id);
            if (!level) throw new ApiError(404, 'LEVEL_NOT_FOUND');
            if (level.authorId !== identity.id) throw new ApiError(403, 'NOT_LEVEL_AUTHOR');
            if (!['public', 'retired'].includes(level.visibility))
                throw new ApiError(409, 'LEVEL_NOT_COPYABLE');
            const packageValue = await readPackage(level);
            const copyName = Array.from(`Copia de ${level.name}`)
                .slice(0, 40)
                .join('')
                .replace(/[^\p{L}\p{N}]+$/u, '');
            return {
                sourcePublicationId: id,
                name: copyName,
                definition: packageValue.definition,
                music: packageValue.music
            };
        }
    };
}

export const publicationInternals = {
    canonicalText,
    hashValue,
    stableLevelId
};
