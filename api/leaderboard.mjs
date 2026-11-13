import { createHash } from 'node:crypto';
import { ApiError } from './identity.mjs';
import { calculateScore, RULES_VERSION } from './scoring.mjs';

export const CAPTURE_FORMAT_VERSION = 'run-v1';
export const PHYSICS_VERSION = 'custom-physics-v1';

const MAX_ENTRIES = 5;
const MAX_RECEIPTS = 32;
const MAX_ATTEMPTS = 8;
const OFFICIAL_LEVELS = new Set(['official:1', 'official:2', 'official:3', 'official:4']);
const PUBLIC_LEVEL = /^public:([0-9a-f]{8}-[0-9a-f]{4}-[1-8][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12})$/;
const ATTEMPT_ID = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const STAT_KEYS = [
    'driverTimeMs',
    'initialParts',
    'partsDelivered',
    'passengersCollected',
    'passengersDelivered'
];
const BODY_KEYS = [
    'attemptId',
    'formatVersion',
    'levelId',
    'physicsVersion',
    'rulesVersion',
    'statistics'
];

const exactKeys = (value, keys) =>
    value &&
    typeof value === 'object' &&
    !Array.isArray(value) &&
    JSON.stringify(Object.keys(value).sort()) === JSON.stringify(keys);

function parseLevelId(value) {
    if (typeof value !== 'string') throw new ApiError(400, 'INVALID_LEVEL');
    if (OFFICIAL_LEVELS.has(value)) return { levelId: value, publicId: null };
    const match = value.match(PUBLIC_LEVEL);
    if (!match) throw new ApiError(400, 'INVALID_LEVEL');
    return { levelId: value, publicId: match[1] };
}

function validateStatistics(value) {
    if (!exactKeys(value, STAT_KEYS)) throw new ApiError(400, 'INVALID_STATISTICS');
    if (!Object.values(value).every(Number.isSafeInteger))
        throw new ApiError(400, 'INVALID_STATISTICS');
    if (
        value.initialParts < 1 ||
        value.initialParts > 256 ||
        value.passengersCollected < 0 ||
        value.passengersCollected > 64 ||
        value.passengersDelivered < 0 ||
        value.passengersDelivered > value.passengersCollected ||
        value.partsDelivered < 0 ||
        value.partsDelivered > value.initialParts ||
        value.driverTimeMs < 0 ||
        value.driverTimeMs > 3600000
    ) {
        throw new ApiError(400, 'INVALID_STATISTICS');
    }
    const score = calculateScore(value);
    if (!score.valid) throw new ApiError(400, 'INVALID_STATISTICS');
    return score;
}

function parseSubmission(body) {
    if (!exactKeys(body, BODY_KEYS)) throw new ApiError(400, 'INVALID_ATTEMPT');
    const level = parseLevelId(body.levelId);
    if (typeof body.attemptId !== 'string' || !ATTEMPT_ID.test(body.attemptId))
        throw new ApiError(400, 'INVALID_ATTEMPT');
    if (body.rulesVersion !== RULES_VERSION)
        throw new ApiError(409, 'RULES_VERSION_MISMATCH');
    if (body.physicsVersion !== PHYSICS_VERSION)
        throw new ApiError(409, 'PHYSICS_VERSION_MISMATCH');
    if (body.formatVersion !== CAPTURE_FORMAT_VERSION)
        throw new ApiError(409, 'FORMAT_VERSION_MISMATCH');
    return { ...level, score: validateStatistics(body.statistics) };
}

function compareEntries(first, second) {
    const order = second.score.total - first.score.total ||
        first.statistics.driverTimeMs - second.statistics.driverTimeMs ||
        first.statistics.initialParts - second.statistics.initialParts ||
        first.submittedAt - second.submittedAt;
    if (order) return order;
    return first.combiId < second.combiId ? -1 : first.combiId > second.combiId ? 1 : 0;
}

function fingerprint(identityId, body) {
    return createHash('sha256')
        .update(JSON.stringify([
            identityId,
            body.levelId,
            body.attemptId,
            body.rulesVersion,
            body.physicsVersion,
            body.formatVersion,
            STAT_KEYS.map((key) => body.statistics[key])
        ]))
        .digest('hex');
}

function readBoard(item) {
    if (!item) return { revision: 0, entries: [], receipts: [] };
    if (
        !Number.isSafeInteger(item.revision) ||
        item.revision < 1 ||
        !Array.isArray(item.entries) ||
        item.entries.length > MAX_ENTRIES ||
        !Array.isArray(item.receipts) ||
        item.receipts.length > MAX_RECEIPTS
    ) {
        throw new Error('Invalid leaderboard item');
    }
    return {
        revision: item.revision,
        entries: item.entries,
        receipts: item.receipts
    };
}

function submissionState(board, identityId, attemptId, idempotent) {
    const ownIndex = board.entries.findIndex((entry) => entry.combiId === identityId);
    const ownEntry = ownIndex >= 0 ? board.entries[ownIndex] : null;
    return {
        attemptId,
        classified: ownEntry?.attemptId === attemptId,
        rank: ownEntry?.attemptId === attemptId ? ownIndex + 1 : null,
        previousBestRetained: Boolean(ownEntry && ownEntry.attemptId !== attemptId),
        idempotent
    };
}

async function responseFor(store, levelId, board, submission = null) {
    const identities = await store.getIdentities(
        board.entries.map((entry) => entry.combiId)
    );
    const entries = board.entries.map((entry, index) => ({
        rank: index + 1,
        combiId: entry.combiId,
        name: identities.get(entry.combiId)?.name ?? null,
        attemptId: entry.attemptId,
        runRef: entry.runRef,
        submittedAt: entry.submittedAt,
        statistics: entry.statistics,
        score: entry.score
    }));
    return {
        levelId,
        rulesVersion: RULES_VERSION,
        verification: 'statistics-only',
        championRunRef: entries[0]?.runRef ?? null,
        entries,
        ...(submission ? { submission } : {})
    };
}

const retryable = (error) =>
    error?.name === 'ConditionalCheckFailedException' ||
    error?.name === 'TransactionConflictException';

export function createLeaderboardService(
    store,
    clock = () => Date.now(),
    pause = (delay) => new Promise((resolve) => setTimeout(resolve, delay))
) {
    async function validateLevel(level) {
        if (level.publicId && !(await store.isPublicLevel(level.publicId)))
            throw new ApiError(404, 'LEVEL_NOT_FOUND');
    }

    return {
        async get(levelId) {
            const level = parseLevelId(levelId);
            await validateLevel(level);
            return responseFor(store, level.levelId, readBoard(
                await store.getLeaderboard(level.levelId)
            ));
        },

        async submit(identity, body) {
            const parsed = parseSubmission(body);
            await validateLevel(parsed);
            const attemptFingerprint = fingerprint(identity.id, body);
            const submittedAt = clock();

            for (let attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
                const board = readBoard(await store.getLeaderboard(parsed.levelId));
                const receipt = board.receipts.find(
                    (item) => item.attemptId === body.attemptId
                );
                if (receipt) {
                    if (
                        receipt.combiId !== identity.id ||
                        receipt.fingerprint !== attemptFingerprint
                    ) {
                        throw new ApiError(409, 'ATTEMPT_ID_CONFLICT');
                    }
                    return responseFor(
                        store,
                        parsed.levelId,
                        board,
                        submissionState(board, identity.id, body.attemptId, true)
                    );
                }

                const candidate = {
                    combiId: identity.id,
                    attemptId: body.attemptId,
                    runRef: `runs/${parsed.levelId}/${identity.id}/${body.attemptId}`,
                    submittedAt,
                    statistics: body.statistics,
                    score: parsed.score
                };
                const previous = board.entries.find(
                    (entry) => entry.combiId === identity.id
                );
                let entries = board.entries;
                if (!previous || compareEntries(candidate, previous) < 0) {
                    entries = board.entries
                        .filter((entry) => entry.combiId !== identity.id)
                        .concat(candidate)
                        .sort(compareEntries)
                        .slice(0, MAX_ENTRIES);
                }
                const next = {
                    revision: board.revision + 1,
                    entries,
                    receipts: board.receipts.concat({
                        attemptId: body.attemptId,
                        combiId: identity.id,
                        fingerprint: attemptFingerprint,
                        receivedAt: submittedAt
                    }).slice(-MAX_RECEIPTS),
                    updatedAt: clock()
                };
                try {
                    await store.replaceLeaderboard(
                        parsed.levelId,
                        board.revision,
                        next
                    );
                    return responseFor(
                        store,
                        parsed.levelId,
                        next,
                        submissionState(next, identity.id, body.attemptId, false)
                    );
                } catch (error) {
                    if (!retryable(error)) throw error;
                    await pause(10 * (attempt + 1) + Math.random() * 20);
                }
            }
            throw new ApiError(409, 'RETRY_LEADERBOARD');
        }
    };
}

export const leaderboardInternals = {
    compareEntries,
    parseSubmission
};
