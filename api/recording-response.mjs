import { RECORDING_MEDIA_TYPE } from './recordings.mjs';

// a stored run, gzip exactly as it sits in S3, as an API Gateway binary response
export function recordingResponse({ body, recording }, cacheControl) {
    return {
        statusCode: 200,
        headers: {
            'content-type': RECORDING_MEDIA_TYPE,
            'cache-control': cacheControl,
            etag: `"${recording.sha256}"`,
            'x-recording-sha256': recording.sha256,
            'x-recording-format': recording.formatVersion
        },
        isBase64Encoded: true,
        body: body.toString('base64')
    };
}
