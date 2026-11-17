import { createHash } from 'node:crypto';
import { gunzipSync, gzipSync } from 'node:zlib';
import { ApiError } from './identity.mjs';
import { CAPTURE_FORMAT_VERSION, PHYSICS_VERSION, RULES_VERSION } from './leaderboard.mjs';
import { officialLevelNumber } from './level-id.mjs';

export const RECORDING_MEDIA_TYPE = 'application/vnd.tunea-tu-combi.run+gzip';
export const SAMPLE_INTERVAL_MS = 50;
export const MAX_COMPRESSED_BYTES = 2 * 1024 * 1024;
export const MAX_UNCOMPRESSED_BYTES = 32 * 1024 * 1024;
export const ORPHAN_GRACE_MS = 24 * 60 * 60 * 1000;

const UUID = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const PUBLIC_LEVEL = /^public:[0-9a-f]{8}-[0-9a-f]{4}-[1-8][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const isLevelId = (value) => officialLevelNumber(value) !== null || PUBLIC_LEVEL.test(value);

function invalid(code = 'INVALID_RECORDING') {
    throw new ApiError(400, code);
}

// The game is trusted for what is inside a recording. The server only checks
// that it was made with the physics and rules it serves.
function checkRecordingVersions(value) {
    if (!value || typeof value !== 'object' || Array.isArray(value)) invalid();
    if (
        value.formatVersion !== CAPTURE_FORMAT_VERSION ||
        value.physicsVersion !== PHYSICS_VERSION ||
        value.rulesVersion !== RULES_VERSION
    ) throw new ApiError(409, 'RECORDING_VERSION_MISMATCH');
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
        if (!isLevelId(levelId ?? '')) throw new ApiError(400, 'INVALID_LEVEL');
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
            const capture = checkRecordingVersions(decoded.value);
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

        async download(levelId, pinnedAttemptId = null, pinnedSha256 = null) {
            await requireActiveLevel(levelId);
            let recording;
            if (pinnedAttemptId !== null || pinnedSha256 !== null) {
                if (!UUID.test(pinnedAttemptId ?? '') ||
                    !/^[0-9a-f]{64}$/.test(pinnedSha256 ?? '')) {
                    throw new ApiError(400, 'INVALID_RECORDING_REFERENCE');
                }
                recording = {
                    key: `recordings/${encodeURIComponent(levelId)}/${pinnedAttemptId}-${pinnedSha256}.json.gz`,
                    attemptId: pinnedAttemptId,
                    levelId,
                    formatVersion: CAPTURE_FORMAT_VERSION,
                    sha256: pinnedSha256
                };
            } else {
                const board = await store.getLeaderboard(levelId);
                recording = board?.championRecording;
                const champion = board?.entries?.[0];
                if (!recording || !champion || recording.attemptId !== champion.attemptId ||
                    recording.combiId !== champion.combiId) {
                    throw new ApiError(404, 'RECORDING_UNAVAILABLE');
                }
            }
            let body;
            try { body = await objects.getRecording(recording.key); }
            catch (error) {
                if (error?.name === 'NoSuchKey' || error?.$metadata?.httpStatusCode === 404)
                    throw new ApiError(404, 'RECORDING_UNAVAILABLE');
                throw error;
            }
            if (body.length > MAX_COMPRESSED_BYTES ||
                (recording.compressedBytes !== undefined &&
                    body.length !== recording.compressedBytes) ||
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
