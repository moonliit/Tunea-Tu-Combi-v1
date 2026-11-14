import {
    DeleteObjectsCommand,
    GetObjectCommand,
    PutObjectCommand,
    S3Client
} from '@aws-sdk/client-s3';

export function createObjectStore(bucket, client = new S3Client({})) {
    return {
        async putJson(key, value) {
            const body = JSON.stringify(value);
            await client.send(new PutObjectCommand({
                Bucket: bucket,
                Key: key,
                Body: body,
                ContentType: 'application/json; charset=utf-8',
                CacheControl: 'private, no-store'
            }));
        },

        async getJson(key) {
            const result = await client.send(new GetObjectCommand({
                Bucket: bucket,
                Key: key
            }));
            return JSON.parse(await result.Body.transformToString());
        },

        async deleteMany(keys) {
            if (!keys.length) return;
            await client.send(new DeleteObjectsCommand({
                Bucket: bucket,
                Delete: {
                    Objects: keys.map((Key) => ({ Key })),
                    Quiet: true
                }
            }));
        }
    };
}
