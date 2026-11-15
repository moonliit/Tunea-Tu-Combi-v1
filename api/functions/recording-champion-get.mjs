import { container } from '../container.mjs';
import { recordingResponse } from '../recording-response.mjs';
import { route } from '../http.mjs';

// GET /api/v1/levels/{levelId}/leaderboard/champion/recording: the run of
// whoever leads right now. It changes when someone overtakes, so it is not cached.
export const handler = route({
    method: 'GET',
    path: /^\/api\/v1\/levels\/([^/]+)\/leaderboard\/champion\/recording$/,
    run: async ({ params }) => {
        const recordings = await container.recordings();
        return recordingResponse(
            await recordings.download(params[0], null, null),
            'private, no-store'
        );
    }
});
