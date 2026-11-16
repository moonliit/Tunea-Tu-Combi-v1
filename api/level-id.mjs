import { ApiError } from './identity.mjs';

// Same limit as MaxOfficialLevelCount in src/app/LevelCatalog.hpp: the game decides how
// many official levels exist, the server only has to accept their numbers.
export const MAX_OFFICIAL_LEVEL = 20;

// 'official:3' -> 3. Anything else ('official:21', 'official:03', 'public:...') -> null.
export function officialLevelNumber(levelId) {
    const match = /^official:([1-9]\d?)$/.exec(levelId);
    const number = match ? Number(match[1]) : 0;
    return number >= 1 && number <= MAX_OFFICIAL_LEVEL ? number : null;
}

// A level is named 'official:<n>' or 'public:<uuid>' in every URL. Only a
// published level exists on the server; the bare uuid is what the services use.
export function publicIdOf(levelId) {
    if (!levelId.startsWith('public:')) throw new ApiError(404, 'LEVEL_NOT_FOUND');
    return levelId.slice('public:'.length);
}
