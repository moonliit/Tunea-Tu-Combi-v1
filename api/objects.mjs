import {
    DeleteObjectsCommand,
    GetObjectCommand,
    GetObjectTaggingCommand,
    ListObjectsV2Command,
    PutObjectTaggingCommand,
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

        async putRecording(key, body, metadata) {
            await client.send(new PutObjectCommand({
                Bucket: bucket,
                Key: key,
                Body: body,
                ContentType: 'application/vnd.tunea-tu-combi.run+gzip',
                CacheControl: 'private, no-store',
                Metadata: {
                    attempt: metadata.attemptId,
                    combi: metadata.combiId,
                    format: metadata.formatVersion,
                    physics: metadata.physicsVersion,
                    rules: metadata.rulesVersion,
                    samplems: String(metadata.sampleIntervalMs),
                    sha256: metadata.sha256
                },
                // An upload is an orphan until DynamoDB links it. A scheduled
                // sweeper reads delete-after; it never deletes a current object.
                Tagging: `retention=orphan&delete-after=${Date.now() + 86400000}`
            }));
        },

        async getRecording(key) {
            const result = await client.send(new GetObjectCommand({ Bucket: bucket, Key: key }));
            return Buffer.from(await result.Body.transformToByteArray());
        },

        async markRecordingCurrent(key) {
            await client.send(new PutObjectTaggingCommand({
                Bucket: bucket,
                Key: key,
                Tagging: { TagSet: [{ Key: 'retention', Value: 'current' }] }
            }));
        },

        async markRecordingOrphan(key, deleteAfter) {
            await client.send(new PutObjectTaggingCommand({
                Bucket: bucket,
                Key: key,
                Tagging: { TagSet: [
                    { Key: 'retention', Value: 'orphan' },
                    { Key: 'delete-after', Value: String(deleteAfter) }
                ] }
            }));
        },

        async listRecordingCleanupCandidates(now, continuationToken = undefined) {
            const page = await client.send(new ListObjectsV2Command({
                Bucket: bucket,
                Prefix: 'recordings/',
                MaxKeys: 100,
                ContinuationToken: continuationToken
            }));
            const candidates = [];
            for (const item of page.Contents ?? []) {
                const tags = await client.send(new GetObjectTaggingCommand({
                    Bucket: bucket,
                    Key: item.Key
                }));
                const values = new Map((tags.TagSet ?? []).map((tag) => [tag.Key, tag.Value]));
                const orphan = values.get('retention') === 'orphan';
                const due = orphan && Number(values.get('delete-after')) <= now;
                if (values.get('retention') === 'current' || due) {
                    candidates.push({ key: item.Key, orphan, due });
                }
            }
            return { items: candidates, cursor: page.NextContinuationToken };
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
