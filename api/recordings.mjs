import { createHash } from 'node:crypto';
import { gunzipSync, gzipSync } from 'node:zlib';
import { ApiError } from './identity.mjs';
import {
    CAPTURE_FORMAT_VERSION,
    PHYSICS_VERSION,
    validateStatistics
} from './leaderboard.mjs';
import { RULES_VERSION } from './scoring.mjs';

export const RECORDING_MEDIA_TYPE = 'application/vnd.tunea-tu-combi.run+gzip';
export const SAMPLE_INTERVAL_MS = 50;
export const MAX_RECORDING_DURATION_MS = 10 * 60 * 1000;
export const MAX_COMPRESSED_BYTES = 2 * 1024 * 1024;
export const MAX_UNCOMPRESSED_BYTES = 32 * 1024 * 1024;
export const ORPHAN_GRACE_MS = 24 * 60 * 60 * 1000;

const UUID = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const LEVEL = /^(official:[1-4]|public:[0-9a-f]{8}-[0-9a-f]{4}-[1-8][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12})$/;
const exactKeys = (value, keys) => value && typeof value === 'object' &&
    !Array.isArray(value) &&
    JSON.stringify(Object.keys(value).sort()) === JSON.stringify([...keys].sort());
const integer = (value, minimum, maximum) => Number.isSafeInteger(value) &&
    value >= minimum && value <= maximum;

function invalid(code = 'INVALID_RECORDING') {
    throw new ApiError(400, code);
}

// This validator DELIBERATELY VALIDATES THE TRACE, not the physics
// A recording NEVER RE-SIMULATES CONTROLS.
export function validateRecordingCapture(value, expected) {
    if (!value || typeof value !== 'object' || Array.isArray(value)) invalid();
    if (
        value.formatVersion !== CAPTURE_FORMAT_VERSION ||
        value.physicsVersion !== PHYSICS_VERSION ||
        value.rulesVersion !== RULES_VERSION
    ) throw new ApiError(409, 'RECORDING_VERSION_MISMATCH');
    if (
        !UUID.test(value.attemptId ?? '') ||
        typeof value.combiId !== 'string' ||
        !LEVEL.test(value.levelId ?? '') ||
        value.attemptId !== expected.attemptId ||
        value.combiId !== expected.combiId ||
        value.levelId !== expected.levelId
    ) invalid();
    if (
        !integer(value.finishedAtMs, 3000, MAX_RECORDING_DURATION_MS) ||
        !Array.isArray(value.construction) || value.construction.length < 1 ||
        value.construction.length > 256 ||
        !integer(value.passengerCount, 0, 64) ||
        !Array.isArray(value.connections) || value.connections.length > 512 ||
        !Array.isArray(value.events) || value.events.length > 4096 ||
        !Array.isArray(value.frames) || value.frames.length < 2 ||
        value.frames.length > MAX_RECORDING_DURATION_MS / SAMPLE_INTERVAL_MS + 2
    ) invalid();

    const score = validateStatistics(value.statistics);
    if (!['driverTimeMs', 'initialParts', 'partsDelivered', 'passengersCollected',
        'passengersDelivered'].every((key) =>
        value.statistics?.[key] === expected.statistics?.[key])) invalid();
    if (!value.score || ![
        'valid', 'passengerPickup', 'passengerDelivery', 'partDelivery',
        'constructionPenalty', 'speed', 'total'
    ].every((key) => value.score[key] === score[key])) invalid();

    const parts = new Set();
    for (const part of value.construction) {
        if (!exactKeys(part, ['containedBy', 'gridX', 'gridY', 'id', 'orientation', 'type']) ||
            !integer(part.id, 1, 0xffffffff) || parts.has(part.id) ||
            !integer(part.type, 0, 6) || !integer(part.orientation, 0, 3) ||
            !integer(part.containedBy, 0, 0xffffffff) ||
            !integer(part.gridX, -4096, 4096) || !integer(part.gridY, -4096, 4096)) invalid();
        parts.add(part.id);
    }
    if (value.statistics.initialParts !== parts.size) invalid();
    for (const part of value.construction) {
        if (part.containedBy && !parts.has(part.containedBy)) invalid();
    }
    const drivers = value.construction.filter((part) => part.type === 5);
    const driverBoxes = new Set(
        value.construction.filter((part) => part.type === 6).map((part) => part.id)
    );
    if (drivers.length !== 1 || !driverBoxes.has(drivers[0].containedBy)) invalid();
    for (const connection of value.connections) {
        if (!exactKeys(connection, ['first', 'second']) ||
            !parts.has(connection.first) || !parts.has(connection.second) ||
            connection.first === connection.second) invalid();
    }

    let previousEvent = -1;
    const pickedPassengers = new Set();
    for (const event of value.events) {
        if (!event || !integer(event.timeMs, 0, value.finishedAtMs) ||
            event.timeMs < previousEvent ||
            !['appear', 'detach', 'disappear', 'passengerPickup'].includes(event.type)) invalid();
        previousEvent = event.timeMs;
        if (event.type === 'detach') {
            if (!exactKeys(event, ['first', 'second', 'timeMs', 'type']) ||
                !parts.has(event.first) || !parts.has(event.second)) invalid();
        } else if (event.type === 'passengerPickup') {
            if (!exactKeys(event, event.hostId === undefined
                ? ['id', 'timeMs', 'type']
                : ['hostId', 'id', 'timeMs', 'type']) ||
                !integer(event.id, 0, Math.max(0, value.passengerCount - 1)) ||
                value.passengerCount === 0 || pickedPassengers.has(event.id) ||
                (event.hostId !== undefined && !parts.has(event.hostId))) invalid();
            pickedPassengers.add(event.id);
        } else if (!exactKeys(event, ['id', 'timeMs', 'type']) ||
            !parts.has(event.id)) invalid();
    }
    if (pickedPassengers.size !== value.statistics.passengersCollected) invalid();

    let previousTime = -1;
    for (const frame of value.frames) {
        if (!exactKeys(frame, ['parts', 'timeMs']) ||
            !integer(frame.timeMs, 0, value.finishedAtMs) ||
            frame.timeMs < previousTime || !Array.isArray(frame.parts) ||
            frame.parts.length > parts.size) invalid();
        // Note after testing:
        // A gap larger than 250 ms makes interpolation visibly inaccurate and is
        // treated as a truncated trace.
        // The forced final frame is exempt only from landing exactly on the 50 ms cadence.
        if (previousTime >= 0 && frame.timeMs - previousTime > 250) invalid('TRUNCATED_RECORDING');
        previousTime = frame.timeMs;
        const frameIds = new Set();
        for (const state of frame.parts) {
            if (!exactKeys(state, ['id', 'rotationMilli', 'xMilli', 'yMilli']) ||
                !parts.has(state.id) || frameIds.has(state.id) ||
                !integer(state.xMilli, -100000000, 100000000) ||
                !integer(state.yMilli, -100000000, 100000000) ||
                !integer(state.rotationMilli, -100000000, 100000000)) invalid();
            frameIds.add(state.id);
        }
    }
    if (value.frames[0].timeMs !== 0 || previousTime !== value.finishedAtMs ||
        value.finishedAtMs - value.statistics.driverTimeMs < 3000 ||
        value.finishedAtMs - value.statistics.driverTimeMs > 3400) {
        invalid('TRUNCATED_RECORDING');
    }
    return value;
}

export function encodeRecording(capture) {
    const plain = Buffer.from(JSON.stringify(capture));
    if (plain.length > MAX_UNCOMPRESSED_BYTES) invalid('RECORDING_TOO_LARGE');
    const compressed = gzipSync(plain, { level: 9 });
    if (compressed.length > MAX_COMPRESSED_BYTES) invalid('RECORDING_TOO_LARGE');
    return {
        body: compressed,
        sha256: createHash('sha256').update(compressed).digest('hex'),
        compressedBytes: compressed.length,
        uncompressedBytes: plain.length
    };
}

export function decodeRecording(body) {
    const compressed = Buffer.from(body);
    if (!compressed.length || compressed.length > MAX_COMPRESSED_BYTES)
        invalid('RECORDING_TOO_LARGE');
    let plain;
    try { plain = gunzipSync(compressed, { maxOutputLength: MAX_UNCOMPRESSED_BYTES }); }
    catch { invalid('INVALID_RECORDING_COMPRESSION'); }
    if (plain.length > MAX_UNCOMPRESSED_BYTES) invalid('RECORDING_TOO_LARGE');
    try { return { value: JSON.parse(plain.toString('utf8')), plainBytes: plain.length }; }
    catch { invalid(); }
}

export function createRecordingService(store, objects, clock = () => Date.now()) {
    async function requireActiveLevel(levelId) {
        if (!LEVEL.test(levelId ?? '')) throw new ApiError(400, 'INVALID_LEVEL');
        if (levelId.startsWith('public:') &&
            !(await store.isPublicLevel(levelId.slice('public:'.length)))) {
            throw new ApiError(404, 'LEVEL_NOT_FOUND');
        }
    }
    return {
        async upload(identity, levelId, attemptId, body, contentType) {
            await requireActiveLevel(levelId);
            if (contentType !== RECORDING_MEDIA_TYPE) throw new ApiError(415, 'RECORDING_GZIP_REQUIRED');
            const board = await store.getLeaderboard(levelId);
            const champion = board?.entries?.[0];
            if (!champion || champion.combiId !== identity.id || champion.attemptId !== attemptId)
                throw new ApiError(409, 'NOT_CURRENT_CHAMPION');
            const decoded = decodeRecording(body);
            const capture = validateRecordingCapture(decoded.value, {
                levelId, attemptId, combiId: identity.id, statistics: champion.statistics
            });
            const compressed = Buffer.from(body);
            const sha256 = createHash('sha256').update(compressed).digest('hex');
            const key = `recordings/${encodeURIComponent(levelId)}/${attemptId}-${sha256}.json.gz`;
            const metadata = {
                key,
                attemptId,
                combiId: identity.id,
                levelId,
                formatVersion: capture.formatVersion,
                physicsVersion: capture.physicsVersion,
                rulesVersion: capture.rulesVersion,
                encoding: 'gzip',
                sampleIntervalMs: SAMPLE_INTERVAL_MS,
                sha256,
                compressedBytes: compressed.length,
                uncompressedBytes: decoded.plainBytes,
                durationMs: capture.finishedAtMs,
                createdAt: clock()
            };
            await objects.putRecording(key, compressed, metadata);
            try {
                const result = await store.attachChampionRecording(
                    levelId, board.revision, identity.id, attemptId, metadata
                );
                await objects.markRecordingCurrent(key).catch(() => {});
                if (result?.previous?.key && result.previous.key !== key) {
                    await objects.markRecordingOrphan(
                        result.previous.key, clock() + ORPHAN_GRACE_MS
                    ).catch(() => {});
                }
                return { status: 'ready', idempotent: result?.idempotent ?? false, recording: metadata };
            } catch (error) {
                await objects.markRecordingOrphan(key, clock() + ORPHAN_GRACE_MS).catch(() => {});
                if (levelId.startsWith('public:') &&
                    !(await store.isPublicLevel(levelId.slice('public:'.length)))) {
                    throw new ApiError(404, 'LEVEL_NOT_FOUND');
                }
                if (error?.name === 'ConditionalCheckFailedException' ||
                    error?.name === 'TransactionCanceledException')
                    throw new ApiError(409, 'NOT_CURRENT_CHAMPION');
                throw error;
            }
        },

        async download(levelId) {
            await requireActiveLevel(levelId);
            const board = await store.getLeaderboard(levelId);
            const recording = board?.championRecording;
            const champion = board?.entries?.[0];
            if (!recording || !champion || recording.attemptId !== champion.attemptId ||
                recording.combiId !== champion.combiId) {
                throw new ApiError(404, 'RECORDING_UNAVAILABLE');
            }
            const body = await objects.getRecording(recording.key);
            if (body.length !== recording.compressedBytes ||
                createHash('sha256').update(body).digest('hex') !== recording.sha256) {
                throw new ApiError(409, 'TRUNCATED_RECORDING');
            }
            return { body, recording };
        },

        // Deploy:
        // Invoked by a scheduled EventBridge event.
        // Every deletion is checked against DynamoDB again,
        // so a stale/missing S3 tag cannot delete the
        // recording currently linked to a champion
        async cleanupDue() {
            let cursor;
            let removed = 0;
            do {
                const page = await objects.listRecordingCleanupCandidates(clock(), cursor);
                cursor = page.cursor;
                const deletable = [];
                for (const item of page.items) {
                    const key = item.key;
                    const match = key.match(/^recordings\/([^/]+)\//);
                    let current = false;
                    if (match) {
                        let levelId;
                        try { levelId = decodeURIComponent(match[1]); } catch {}
                        if (levelId) {
                            const board = await store.getLeaderboard(levelId);
                            current = board?.championRecording?.key === key;
                        }
                    }
                    if (current) {
                        if (item.orphan)
                            await objects.markRecordingCurrent(key).catch(() => {});
                    } else if (item.due) {
                        deletable.push(key);
                    } else {
                        await objects.markRecordingOrphan(
                            key, clock() + ORPHAN_GRACE_MS
                        ).catch(() => {});
                    }
                }
                await objects.deleteMany(deletable);
                removed += deletable.length;
            } while (cursor);
            return { removed };
        }
    };
}
