import { createHash } from 'node:crypto';
import { ApiError } from './identity.mjs';
import {
    CAPTURE_FORMAT_VERSION,
    PHYSICS_VERSION,
    attemptFingerprint,
    validateStatistics
} from './leaderboard.mjs';
import { RULES_VERSION } from './scoring.mjs';

const MAX_ACTIVE_PUBLICATIONS = 5;
const CATALOG_PAGE_SIZE = 10;
const SEARCH_SCOPE = 100;
const MAX_CAPTURE_MS = 600000;
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

function validateName(value) {
    if (typeof value !== 'string' || value.length > 256 || /[\p{C}\p{Zl}\p{Zp}]/u.test(value))
        throw new ApiError(400, 'INVALID_LEVEL_NAME');
    const name = value.normalize('NFKC').replace(/\p{Zs}+/gu, ' ').trim();
    const chars = Array.from(name);
    if (
        chars.length < 3 ||
        chars.length > 40 ||
        !/^[\p{L}\p{N}][\p{L}\p{N} '\-]*[\p{L}\p{N}]$/u.test(name)
    ) {
        throw new ApiError(400, 'INVALID_LEVEL_NAME');
    }
    return name;
}

function finiteNumber(value, minimum = -100000, maximum = 100000) {
    if (typeof value !== 'number' || !Number.isFinite(value) || value < minimum || value > maximum)
        throw new ApiError(400, 'INVALID_LEVEL');
    return Object.is(value, -0) ? 0 : value;
}

function integer(value, minimum, maximum, code = 'INVALID_LEVEL') {
    if (!Number.isSafeInteger(value) || value < minimum || value > maximum)
        throw new ApiError(400, code);
    return value;
}

function point(value) {
    if (!exactKeys(value, ['x', 'y'])) throw new ApiError(400, 'INVALID_LEVEL');
    return { x: finiteNumber(value.x), y: finiteNumber(value.y) };
}

export function validateLevelDefinition(value) {
    const keys = Object.keys(value ?? {}).sort();
    const required = ['goal', 'grid', 'passengers', 'terrain'];
    if (
        !value ||
        typeof value !== 'object' ||
        Array.isArray(value) ||
        !required.every((key) => keys.includes(key)) ||
        keys.some((key) => ![...required, 'initialCamera'].includes(key))
    ) {
        throw new ApiError(400, 'INVALID_LEVEL');
    }
    if (!exactKeys(value.terrain, ['polygons']) || !Array.isArray(value.terrain.polygons))
        throw new ApiError(400, 'INVALID_LEVEL');
    if (value.terrain.polygons.length < 1 || value.terrain.polygons.length > 32)
        throw new ApiError(400, 'INVALID_LEVEL');

    let segmentCount = 0;
    let pointCount = 0;
    const polygons = value.terrain.polygons.map((polygon) => {
        if (!exactKeys(polygon, ['segments']) || !Array.isArray(polygon.segments))
            throw new ApiError(400, 'INVALID_LEVEL');
        if (polygon.segments.length < 1 || polygon.segments.length > 32)
            throw new ApiError(400, 'INVALID_LEVEL');
        segmentCount += polygon.segments.length;
        const segments = polygon.segments.map((segment) => {
            if (!exactKeys(segment, ['points', 'resolution', 'type']))
                throw new ApiError(400, 'INVALID_LEVEL');
            if (!['Line', 'CatmullRom'].includes(segment.type) || !Array.isArray(segment.points))
                throw new ApiError(400, 'INVALID_LEVEL');
            if (segment.points.length < 2 || segment.points.length > 128)
                throw new ApiError(400, 'INVALID_LEVEL');
            pointCount += segment.points.length;
            return {
                type: segment.type,
                points: segment.points.map(point),
                resolution: integer(segment.resolution, 1, 32)
            };
        });
        const samePoint = (first, second) =>
            first.x === second.x && first.y === second.y;
        for (let index = 1; index < segments.length; index++) {
            if (!samePoint(segments[index - 1].points.at(-1), segments[index].points[0]))
                throw new ApiError(400, 'INVALID_LEVEL');
        }
        if (!samePoint(segments[0].points[0], segments.at(-1).points.at(-1)))
            throw new ApiError(400, 'INVALID_LEVEL');
        const vertices = new Set(
            segments.flatMap((segment) => segment.points)
                .map((value) => `${value.x},${value.y}`)
        );
        if (vertices.size < 3) throw new ApiError(400, 'INVALID_LEVEL');
        return { segments };
    });
    if (segmentCount > 64 || pointCount > 1024) throw new ApiError(400, 'INVALID_LEVEL');

    if (!exactKeys(value.grid, ['cellSize', 'dims', 'worldPos']))
        throw new ApiError(400, 'INVALID_LEVEL');
    if (!Array.isArray(value.grid.dims) || value.grid.dims.length !== 2)
        throw new ApiError(400, 'INVALID_LEVEL');
    const dims = [
        integer(value.grid.dims[0], 1, 16),
        integer(value.grid.dims[1], 1, 16)
    ];
    if (dims[0] * dims[1] > 128) throw new ApiError(400, 'INVALID_LEVEL');
    const grid = {
        dims,
        worldPos: point(value.grid.worldPos),
        cellSize: finiteNumber(value.grid.cellSize, 10, 200)
    };
    if (!exactKeys(value.goal, ['position'])) throw new ApiError(400, 'INVALID_LEVEL');
    const goal = { position: point(value.goal.position) };
    if (goal.position.x <= grid.worldPos.x)
        throw new ApiError(400, 'GOAL_MUST_BE_RIGHT_OF_GRID');

    if (!Array.isArray(value.passengers) || value.passengers.length > 64)
        throw new ApiError(400, 'INVALID_LEVEL');
    const passengers = value.passengers.map((passenger) => {
        if (!exactKeys(passenger, ['hasSkirt', 'position']) || typeof passenger.hasSkirt !== 'boolean')
            throw new ApiError(400, 'INVALID_LEVEL');
        return { position: point(passenger.position), hasSkirt: passenger.hasSkirt };
    });
    const result = { terrain: { polygons }, grid, goal, passengers };
    if (value.initialCamera !== undefined) {
        if (!exactKeys(value.initialCamera, ['target', 'zoom']))
            throw new ApiError(400, 'INVALID_LEVEL');
        result.initialCamera = {
            target: point(value.initialCamera.target),
            zoom: finiteNumber(value.initialCamera.zoom, 0.1, 5)
        };
    }
    return canonical(result);
}

function validateMusic(value) {
    if (!exactKeys(value, ['artist', 'path', 'title']))
        throw new ApiError(400, 'INVALID_MUSIC');
    if (
        typeof value.path !== 'string' ||
        value.path.length > 180 ||
        !/^assets\/audio\/custom\/[\p{L}\p{N} .,'()!_\-&]+\.(?:ogg|wav)$/u.test(value.path) ||
        typeof value.title !== 'string' ||
        Array.from(value.title).length > 80 ||
        typeof value.artist !== 'string' ||
        Array.from(value.artist).length > 80 ||
        /[\p{C}]/u.test(value.title + value.artist)
    ) {
        throw new ApiError(400, 'INVALID_MUSIC');
    }
    return {
        path: value.path.normalize('NFKC'),
        title: value.title.normalize('NFKC'),
        artist: value.artist.normalize('NFKC')
    };
}

function validateCapture(value, identityId, draft) {
    const definition = draft.definition;
    const captureKeys = [
        'attemptId', 'combiId', 'connections', 'construction', 'events',
        'draftSnapshot', 'finishedAtMs', 'formatVersion', 'frames',
        'levelDefinition', 'levelId',
        'passengerCount', 'physicsVersion', 'rulesVersion', 'score', 'statistics'
    ];
    if (!exactKeys(value, captureKeys)) throw new ApiError(400, 'INVALID_CAPTURE');
    if (!UUID.test(value.attemptId) || value.combiId !== identityId || value.levelId !== '')
        throw new ApiError(400, 'INVALID_CAPTURE');
    if (
        value.formatVersion !== CAPTURE_FORMAT_VERSION ||
        value.physicsVersion !== PHYSICS_VERSION ||
        value.rulesVersion !== RULES_VERSION
    ) {
        throw new ApiError(409, 'CAPTURE_VERSION_MISMATCH');
    }
    const capturedDefinition = validateLevelDefinition(value.levelDefinition);
    if (canonicalText(capturedDefinition) !== canonicalText(definition))
        throw new ApiError(409, 'DRAFT_CHANGED_AFTER_WIN');
    if (!exactKeys(value.draftSnapshot, ['definition', 'music', 'name']))
        throw new ApiError(400, 'INVALID_CAPTURE');
    const capturedDraft = {
        name: validateName(value.draftSnapshot.name),
        definition: validateLevelDefinition(value.draftSnapshot.definition),
        music: validateMusic(value.draftSnapshot.music)
    };
    if (canonicalText(capturedDraft) !== canonicalText(draft))
        throw new ApiError(409, 'DRAFT_CHANGED_AFTER_WIN');
    integer(value.passengerCount, 0, 64, 'INVALID_CAPTURE');
    if (value.passengerCount !== definition.passengers.length)
        throw new ApiError(400, 'INVALID_CAPTURE');

    const score = validateStatistics(value.statistics);
    if (!exactKeys(value.score, [
        'constructionPenalty', 'partDelivery', 'passengerDelivery',
        'passengerPickup', 'speed', 'total', 'valid'
    ]) || canonicalText(value.score) !== canonicalText(score)) {
        throw new ApiError(400, 'INVALID_CAPTURE');
    }
    if (!Array.isArray(value.construction) || value.construction.length < 1 || value.construction.length > 256)
        throw new ApiError(400, 'INVALID_CAPTURE');
    if (value.statistics.initialParts !== value.construction.length)
        throw new ApiError(400, 'INVALID_CAPTURE');

    const parts = new Map();
    for (const part of value.construction) {
        if (!exactKeys(part, ['containedBy', 'gridX', 'gridY', 'id', 'orientation', 'type']))
            throw new ApiError(400, 'INVALID_CAPTURE');
        integer(part.id, 1, 0xffffffff, 'INVALID_CAPTURE');
        if (parts.has(part.id)) throw new ApiError(400, 'INVALID_CAPTURE');
        integer(part.type, 0, 6, 'INVALID_CAPTURE');
        integer(part.gridX, 0, definition.grid.dims[0] - 1, 'INVALID_CAPTURE');
        integer(part.gridY, 0, definition.grid.dims[1] - 1, 'INVALID_CAPTURE');
        integer(part.orientation, 0, 3, 'INVALID_CAPTURE');
        integer(part.containedBy, 0, 0xffffffff, 'INVALID_CAPTURE');
        parts.set(part.id, part);
    }
    const drivers = [...parts.values()].filter((part) => part.type === 5);
    const driverBoxes = [...parts.values()].filter((part) => part.type === 6);
    if (
        drivers.length !== 1 ||
        driverBoxes.length !== 1 ||
        drivers[0].containedBy !== driverBoxes[0].id
    ) {
        throw new ApiError(400, 'INVALID_CAPTURE');
    }
    for (const part of parts.values()) {
        if (part.containedBy !== 0 && !parts.has(part.containedBy))
            throw new ApiError(400, 'INVALID_CAPTURE');
    }
    if (!Array.isArray(value.connections) || value.connections.length > 512)
        throw new ApiError(400, 'INVALID_CAPTURE');
    for (const connection of value.connections) {
        if (
            !exactKeys(connection, ['first', 'second']) ||
            !parts.has(connection.first) ||
            !parts.has(connection.second) ||
            connection.first === connection.second
        ) {
            throw new ApiError(400, 'INVALID_CAPTURE');
        }
    }
    integer(value.finishedAtMs, 3000, MAX_CAPTURE_MS, 'INVALID_CAPTURE');
    if (!Array.isArray(value.events) || value.events.length > 2048)
        throw new ApiError(400, 'INVALID_CAPTURE');
    let previousEventTime = -1;
    const pickedPassengers = new Set();
    for (const event of value.events) {
        if (!event || typeof event !== 'object' || Array.isArray(event))
            throw new ApiError(400, 'INVALID_CAPTURE');
        integer(event.timeMs, 0, MAX_CAPTURE_MS, 'INVALID_CAPTURE');
        if (event.timeMs < previousEventTime || event.timeMs > value.finishedAtMs)
            throw new ApiError(400, 'INVALID_CAPTURE');
        previousEventTime = event.timeMs;
        if (!['appear', 'detach', 'disappear', 'passengerPickup'].includes(event.type))
            throw new ApiError(400, 'INVALID_CAPTURE');
        if (event.type === 'detach') {
            if (
                !exactKeys(event, ['first', 'second', 'timeMs', 'type']) ||
                !parts.has(event.first) ||
                !parts.has(event.second)
            ) {
                throw new ApiError(400, 'INVALID_CAPTURE');
            }
        } else if (event.type === 'passengerPickup') {
            if (!exactKeys(event, ['id', 'timeMs', 'type']))
                throw new ApiError(400, 'INVALID_CAPTURE');
            integer(event.id, 0, Math.max(0, value.passengerCount - 1), 'INVALID_CAPTURE');
            if (value.passengerCount === 0) throw new ApiError(400, 'INVALID_CAPTURE');
            if (pickedPassengers.has(event.id)) throw new ApiError(400, 'INVALID_CAPTURE');
            pickedPassengers.add(event.id);
        } else {
            if (!exactKeys(event, ['id', 'timeMs', 'type']) || !parts.has(event.id))
                throw new ApiError(400, 'INVALID_CAPTURE');
        }
    }
    if (pickedPassengers.size !== value.statistics.passengersCollected)
        throw new ApiError(400, 'INVALID_CAPTURE');

    if (!Array.isArray(value.frames) || value.frames.length < 2 || value.frames.length > 12001)
        throw new ApiError(400, 'INVALID_CAPTURE');
    const driverId = drivers[0].id;
    const goalXMilli = Math.round(definition.goal.position.x * 1000);
    let previousTime = -1;
    let firstWindowSample = null;
    let lastWindowSample = null;
    for (const frame of value.frames) {
        if (!exactKeys(frame, ['parts', 'timeMs']) || !Array.isArray(frame.parts) || frame.parts.length > 256)
            throw new ApiError(400, 'INVALID_CAPTURE');
        integer(frame.timeMs, 0, MAX_CAPTURE_MS, 'INVALID_CAPTURE');
        if (frame.timeMs < previousTime) throw new ApiError(400, 'INVALID_CAPTURE');
        previousTime = frame.timeMs;
        let driverState = null;
        const frameIds = new Set();
        for (const state of frame.parts) {
            if (!exactKeys(state, ['id', 'rotationMilli', 'xMilli', 'yMilli']))
                throw new ApiError(400, 'INVALID_CAPTURE');
            if (!parts.has(state.id) || frameIds.has(state.id))
                throw new ApiError(400, 'INVALID_CAPTURE');
            frameIds.add(state.id);
            integer(state.xMilli, -100000000, 100000000, 'INVALID_CAPTURE');
            integer(state.yMilli, -100000000, 100000000, 'INVALID_CAPTURE');
            integer(state.rotationMilli, -100000000, 100000000, 'INVALID_CAPTURE');
            if (state.id === driverId) driverState = state;
        }
        if (frame.timeMs > value.statistics.driverTimeMs) {
            if (!driverState || driverState.xMilli < goalXMilli)
                throw new ApiError(400, 'INVALID_GOAL_WINDOW');
            if (lastWindowSample !== null && frame.timeMs - lastWindowSample > 250)
                throw new ApiError(400, 'INVALID_GOAL_WINDOW');
            firstWindowSample ??= frame.timeMs;
            lastWindowSample = frame.timeMs;
        }
    }
    if (
        previousTime !== value.finishedAtMs ||
        value.finishedAtMs - value.statistics.driverTimeMs < 3000 ||
        value.finishedAtMs - value.statistics.driverTimeMs > 3400 ||
        firstWindowSample === null ||
        firstWindowSample - value.statistics.driverTimeMs > 250 ||
        lastWindowSample < value.statistics.driverTimeMs + 3000
    ) {
        throw new ApiError(400, 'INVALID_GOAL_WINDOW');
    }
    return { capture: canonical(value), score };
}

function validatePublishBody(body, identityId) {
    if (!exactKeys(body, [
        'capture', 'definition', 'draftId', 'music', 'name', 'publicationRequestId'
    ])) {
        throw new ApiError(400, 'INVALID_PUBLICATION');
    }
    if (!UUID.test(body.publicationRequestId) || !UUID.test(body.draftId))
        throw new ApiError(400, 'INVALID_PUBLICATION');
    const definition = validateLevelDefinition(body.definition);
    const name = validateName(body.name);
    const music = validateMusic(body.music);
    const draft = { name, definition, music };
    const run = validateCapture(body.capture, identityId, draft);
    return {
        publicationRequestId: body.publicationRequestId,
        draftId: body.draftId,
        name,
        music,
        definition,
        capture: run.capture,
        score: run.score
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
        const definition = validateLevelDefinition(value.definition);
        const music = validateMusic(value.music);
        if (
            value.levelFingerprint !== level.levelFingerprint ||
            hashValue(definition) !== level.levelFingerprint
        ) {
            throw new Error('Level fingerprint mismatch');
        }
        return { definition, music };
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
            const parsed = validatePublishBody(body, identity.id);
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
            const runKey = `runs/public:${id}/${identity.id}/${parsed.capture.attemptId}-${payloadFingerprint}.json`;
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
                await objects.putJson(runKey, parsed.capture);
                await store.publishLevel({
                    level,
                    board,
                    payloadFingerprint,
                    publicationRequestId: parsed.publicationRequestId
                });
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

        async mine(identity, cursor) {
            let page;
            try {
                page = await store.queryAuthorLevels(identity.id, cursor, 20);
            } catch (error) {
                if (error?.message === 'Invalid cursor')
                    throw new ApiError(400, 'INVALID_CURSOR');
                throw error;
            }
            return {
                activeCount: (await store.getAuthorState(identity.id))?.activePublications ?? 0,
                limit: MAX_ACTIVE_PUBLICATIONS,
                items: page.items.map((item) => ({
                    id: item.levelId,
                    name: item.name,
                    status: item.visibility,
                    publishedAt: item.publishedAt,
                    retiredAt: item.retiredAt ?? null
                })),
                cursor: page.cursor
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
    stableLevelId,
    validateCapture,
    validateName
};
