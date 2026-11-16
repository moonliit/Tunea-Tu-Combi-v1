// Composition root. Every Lambda asks only for the pieces it uses, so a
// function that never touches S3 never needs CONTENT_BUCKET or its SDK client.

const cache = new Map();

function memo(name, build) {
    if (!cache.has(name)) {
        const pending = build().catch((error) => {
            // a failed setup is retried by the next invocation
            cache.delete(name);
            throw error;
        });
        cache.set(name, pending);
    }
    return cache.get(name);
}

function required(name) {
    const value = process.env[name];
    if (!value) throw new Error(`${name} missing`);
    return value;
}

// settings are read before the SDK is loaded, so a missing variable fails fast
const store = () => memo('store', async () => {
    const table = required('TABLE_NAME');
    const { createStore } = await import('./store.mjs');
    return createStore(table);
});

const objectStore = () => memo('objects', async () => {
    const bucket = required('CONTENT_BUCKET');
    const { createObjectStore } = await import('./objects.mjs');
    return createObjectStore(bucket);
});

export const container = {
    publicOrigin: () => process.env.PUBLIC_ORIGIN,

    identity: () => memo('identity', async () => {
        const { createIdentityService } = await import('./identity.mjs');
        return createIdentityService(await store());
    }),

    // objects: false for functions that never read or tag S3 objects
    leaderboard: ({ objects = true } = {}) =>
        memo(`leaderboard:${objects}`, async () => {
            const { createLeaderboardService } = await import('./leaderboard.mjs');
            return createLeaderboardService(
                await store(),
                undefined,
                undefined,
                objects ? await objectStore() : null
            );
        }),

    publications: ({ objects = true } = {}) =>
        memo(`publications:${objects}`, async () => {
            const { createPublicationService } = await import('./publications.mjs');
            return createPublicationService(
                await store(),
                objects ? await objectStore() : null
            );
        }),

    recordings: () => memo('recordings', async () => {
        const { createRecordingService } = await import('./recordings.mjs');
        return createRecordingService(await store(), await objectStore());
    })
};
