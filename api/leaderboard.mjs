import { createHash } from 'node:crypto';
import { ApiError } from './identity.mjs';
import { officialLevelNumber } from './level-id.mjs';
import { createProgressService } from './progress.mjs';

export const CAPTURE_FORMAT_VERSION = 'run-v2';
export const PHYSICS_VERSION = 'custom-physics-v1';
// The score formula lives in the game (src/scoring/ScoreRules.cpp); this only names its version.
export const RULES_VERSION = 'score-v2';

const MAX_ENTRIES = 5;
const MAX_RECEIPTS = 32;
const MAX_ATTEMPTS = 8;
const PUBLIC_LEVEL = /^public:([0-9a-f]{8}-[0-9a-f]{4}-[1-8][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12})$/;
const ATTEMPT_ID = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const STAT_KEYS = [
    'driverTimeMs',
    'initialParts',
    'partsDelivered',
    'passengersCollected',
    'passengersDelivered'
];

function parseLevelId(value) {
    if (typeof value !== 'string') throw new ApiError(400, 'INVALID_LEVEL');
    if (officialLevelNumber(value) !== null) return { levelId: value, publicId: null };
    const match = value.match(PUBLIC_LEVEL);
    if (!match) throw new ApiError(400, 'INVALID_LEVEL');
    return { levelId: value, publicId: match[1] };
}

export const isObject = (value) =>
    Boolean(value) && typeof value === 'object' && !Array.isArray(value);

function parseSubmission(body) {
    const level = parseLevelId(body.levelId);
    if (typeof body.attemptId !== 'string' || !ATTEMPT_ID.test(body.attemptId))
        throw new ApiError(400, 'INVALID_ATTEMPT');
    if (body.rulesVersion !== RULES_VERSION)
        throw new ApiError(409, 'RULES_VERSION_MISMATCH');
    if (body.physicsVersion !== PHYSICS_VERSION)
        throw new ApiError(409, 'PHYSICS_VERSION_MISMATCH');
    if (body.formatVersion !== CAPTURE_FORMAT_VERSION)
        throw new ApiError(409, 'FORMAT_VERSION_MISMATCH');
    // the game calculates the score; the server stores and ranks it as it comes
    if (!isObject(body.statistics) || !isObject(body.score))
        throw new ApiError(400, 'INVALID_ATTEMPT');
    return { ...level, score: body.score };
}

function compareEntries(first, second) {
    const order = second.score.total - first.score.total ||
        first.statistics.driverTimeMs - second.statistics.driverTimeMs ||
        first.statistics.initialParts - second.statistics.initialParts ||
        first.submittedAt - second.submittedAt;
    if (order) return order;
    return first.combiId < second.combiId ? -1 : first.combiId > second.combiId ? 1 : 0;
}

export function attemptFingerprint(identityId, body) {
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
    if (!item) return { revision: 0, entries: [], receipts: [], championRecording: null };
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
        receipts: item.receipts,
        championRecording: item.championRecording ?? null
    };
}

function submissionState(board, identityId, attemptId, idempotent) {
    const ownIndex = board.entries.findIndex((entry) => entry.combiId === identityId);
    const ownEntry = ownIndex >= 0 ? board.entries[ownIndex] : null;
    return {
        attemptId,
        rank: ownEntry?.attemptId === attemptId ? ownIndex + 1 : null,
        previousBestRetained: Boolean(ownEntry && ownEntry.attemptId !== attemptId),
        idempotent
    };
}

async function responseFor(store, levelId, board, submission = null) {
    const identities = await store.getIdentities(
        board.entries.map((entry) => entry.combiId)
    );
    const recording = board.championRecording &&
        board.entries[0]?.attemptId === board.championRecording.attemptId &&
        board.entries[0]?.combiId === board.championRecording.combiId
        ? board.championRecording : null;
    const entries = board.entries.map((entry, index) => ({
        rank: index + 1,
        combiId: entry.combiId,
        name: identities.get(entry.combiId)?.name ?? null,
        attemptId: entry.attemptId,
        runRef: index === 0 && recording ? recording.key : null,
        statistics: entry.statistics,
        score: entry.score
    }));
    return {
        levelId,
        rulesVersion: RULES_VERSION,
        championRunRef: recording?.key ?? null,
        championRecording: recording
            ? { status: 'ready', ...recording }
            : { status: 'unavailable' },
        entries,
        ...(submission ? { submission } : {})
    };
}

const retryable = (error) =>
    error?.name === 'ConditionalCheckFailedException' ||
    error?.name === 'TransactionConflictException' ||
    error?.name === 'TransactionCanceledException';

export function createLeaderboardService(
    store,
    clock = () => Date.now(),
    pause = (delay) => new Promise((resolve) => setTimeout(resolve, delay))
) {
    const progress = createProgressService(store, clock);

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
            // A valid win unlocks the next official level, whatever its place in the ranking.
            const official = officialLevelNumber(parsed.levelId);
            if (official !== null) await progress.unlockAfter(identity.id, official);
            const submissionFingerprint = attemptFingerprint(identity.id, body);
            const submittedAt = clock();

            for (let attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
                const board = readBoard(await store.getLeaderboard(parsed.levelId));
                const receipt = board.receipts.find(
                    (item) => item.attemptId === body.attemptId
                );
                if (receipt) {
                    if (
                        receipt.combiId !== identity.id ||
                        receipt.fingerprint !== submissionFingerprint
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
                    runRef: null,
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
                    championRecording:
                        board.championRecording &&
                        entries[0]?.attemptId === board.championRecording.attemptId &&
                        entries[0]?.combiId === board.championRecording.combiId
                            ? board.championRecording
                            : null,
                    receipts: board.receipts.concat({
                        attemptId: body.attemptId,
                        combiId: identity.id,
                        fingerprint: submissionFingerprint,
                        receivedAt: submittedAt
                    }).slice(-MAX_RECEIPTS),
                    updatedAt: clock()
                };
                try {
                    await store.replaceLeaderboard(
                        parsed.levelId,
                        board.revision,
                        next,
                        parsed.publicId
                    );
                    return responseFor(
                        store,
                        parsed.levelId,
                        next,
                        submissionState(next, identity.id, body.attemptId, false)
                    );
                } catch (error) {
                    if (!retryable(error)) throw error;
                    if (parsed.publicId && !(await store.isPublicLevel(parsed.publicId)))
                        throw new ApiError(404, 'LEVEL_NOT_FOUND');
                    await pause(10 * (attempt + 1) + Math.random() * 20);
                }
            }
            throw new ApiError(409, 'RETRY_LEADERBOARD');
        }
    };
}

export const leaderboardInternals = {
    compareEntries,
    parseSubmission,
    readBoard
};
