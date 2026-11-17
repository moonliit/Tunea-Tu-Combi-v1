import { createHash } from 'node:crypto';
import { gunzipSync, gzipSync } from 'node:zlib';
import { ApiError } from './identity.mjs';
import { CAPTURE_FORMAT_VERSION, PHYSICS_VERSION, RULES_VERSION } from './leaderboard.mjs';
import { officialLevelNumber } from './level-id.mjs';

export const RECORDING_MEDIA_TYPE = 'application/vnd.tunea-tu-combi.run+gzip';
export const MAX_COMPRESSED_BYTES = 2 * 1024 * 1024;
export const MAX_UNCOMPRESSED_BYTES = 32 * 1024 * 1024;

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
        sha256: createHash('sha256').update(compressed).digest('hex')
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
    try { return JSON.parse(plain.toString('utf8')); }
    catch { invalid(); }
}

export function createRecordingService(store, objects) {
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
            const capture = checkRecordingVersions(decodeRecording(body));
            const compressed = Buffer.from(body);
            const sha256 = createHash('sha256').update(compressed).digest('hex');
            const key = `recordings/${encodeURIComponent(levelId)}/${attemptId}-${sha256}.json.gz`;
            const recording = {
                key,
                attemptId,
                combiId: identity.id,
                levelId,
                formatVersion: capture.formatVersion,
                physicsVersion: capture.physicsVersion,
                rulesVersion: capture.rulesVersion,
                sha256
            };
            await objects.putRecording(key, compressed);
            try {
                const result = await store.attachChampionRecording(
                    levelId, board.revision, identity.id, attemptId, recording
                );
                return { status: 'ready', idempotent: result.idempotent, recording };
            } catch (error) {
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

        // A recording is asked for by its attempt and its hash, so what comes back
        // can never change. The game learns both from the leaderboard.
        async download(levelId, attemptId, sha256) {
            await requireActiveLevel(levelId);
            // the key is built from them, so they must be well formed
            if (!UUID.test(attemptId ?? '') || !/^[0-9a-f]{64}$/.test(sha256 ?? ''))
                throw new ApiError(400, 'INVALID_RECORDING_REFERENCE');
            let body;
            try {
                body = await objects.getRecording(
                    `recordings/${encodeURIComponent(levelId)}/${attemptId}-${sha256}.json.gz`
                );
            } catch (error) {
                if (error?.name === 'NoSuchKey' || error?.$metadata?.httpStatusCode === 404)
                    throw new ApiError(404, 'RECORDING_UNAVAILABLE');
                throw error;
            }
            if (createHash('sha256').update(body).digest('hex') !== sha256)
                throw new ApiError(409, 'TRUNCATED_RECORDING');
            return { body, sha256 };
        }
    };
}
