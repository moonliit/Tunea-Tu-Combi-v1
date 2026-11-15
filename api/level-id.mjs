import { ApiError } from './identity.mjs';

// A level is named 'official:<n>' or 'public:<uuid>' in every URL. Only a
// published level exists on the server; the bare uuid is what the services use.
export function publicIdOf(levelId) {
    if (!levelId.startsWith('public:')) throw new ApiError(404, 'LEVEL_NOT_FOUND');
    return levelId.slice('public:'.length);
}
