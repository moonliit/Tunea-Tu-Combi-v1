import { MAX_OFFICIAL_LEVEL } from './level-id.mjs';

// How far a player got in the official levels: the highest one they can play.
export function createProgressService(store, clock = () => Date.now()) {
    return {
        async get(identityId) {
            const item = await store.getProgress(identityId);
            return { unlockedLevel: item?.unlockedLevel ?? 1 };
        },

        // Winning official level n unlocks level n + 1.
        unlockAfter: (identityId, level) =>
            store.raiseProgress(identityId, Math.min(level + 1, MAX_OFFICIAL_LEVEL), clock())
    };
}
