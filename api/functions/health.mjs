import { respond, route } from '../http.mjs';

// GET /api/v1/health: answers without touching DynamoDB or S3
export const handler = route({
    method: 'GET',
    path: '/api/v1/health',
    run: async () => respond(200, { ok: true, service: 'tunea-tu-combi' })
});
