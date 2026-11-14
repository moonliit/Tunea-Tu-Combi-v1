import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import {
    BatchGetCommand,
    DynamoDBDocumentClient,
    GetCommand,
    PutCommand,
    QueryCommand,
    TransactWriteCommand,
    UpdateCommand
} from '@aws-sdk/lib-dynamodb';

// only sessions use expiresAt (TTL), data belongs to COMBI#<id>
export function createStore(table, client = DynamoDBDocumentClient.from(new DynamoDBClient({}))) {
    const key = (PK, SK = 'META') => ({ PK, SK });
    const transact = (...TransactItems) => client.send(
        new TransactWriteCommand({ TransactItems })
    );
    const putOnce = (PK, item, SK = 'META') => ({
        Put: {
            TableName: table,
            Item: { ...key(PK, SK), ...item },
            ConditionExpression: 'attribute_not_exists(PK)'
        }
    });
    const get = async (PK) => (
        await client.send(new GetCommand({
            TableName: table,
            Key: key(PK),
            ConsistentRead: true
        }))
    ).Item;

    const decodeCursor = (cursor, PK) => {
        if (!cursor) return undefined;
        try {
            const value = JSON.parse(Buffer.from(cursor, 'base64url').toString('utf8'));
            if (value.PK !== PK || typeof value.SK !== 'string') throw new Error();
            return key(value.PK, value.SK);
        } catch {
            throw new Error('Invalid cursor');
        }
    };
    const encodeCursor = (value) => value
        ? Buffer.from(JSON.stringify(key(value.PK, value.SK))).toString('base64url')
        : null;

    async function batchGet(keys, options = {}) {
        let pending = [...new Map(
            keys.map((value) => [`${value.PK}\0${value.SK}`, value])
        ).values()];
        const items = [];
        for (let attempt = 0; pending.length && attempt < 4; attempt++) {
            const result = await client.send(new BatchGetCommand({
                RequestItems: {
                    [table]: {
                        Keys: pending,
                        ConsistentRead: true,
                        ...options
                    }
                }
            }));
            items.push(...(result.Responses?.[table] ?? []));
            pending = result.UnprocessedKeys?.[table]?.Keys ?? [];
        }
        return items;
    }

    async function queryPage(PK, cursor, limit, prefix = null) {
        const result = await client.send(new QueryCommand({
            TableName: table,
            KeyConditionExpression: prefix
                ? 'PK = :pk AND begins_with(SK, :prefix)'
                : 'PK = :pk',
            ExpressionAttributeValues: {
                ':pk': PK,
                ...(prefix ? { ':prefix': prefix } : {})
            },
            ExclusiveStartKey: decodeCursor(cursor, PK),
            ConsistentRead: true,
            ScanIndexForward: false,
            Limit: limit
        }));
        return {
            items: result.Items ?? [],
            cursor: encodeCursor(result.LastEvaluatedKey)
        };
    }

    return {
        get,

        async getIdentities(ids) {
            const items = await batchGet(
                ids.map((id) => key(`COMBI#${id}`)),
                {
                    ProjectionExpression: 'PK, id, #name',
                    ExpressionAttributeNames: { '#name': 'name' }
                }
            );
            return new Map(items.map((item) => [item.id, item]));
        },

        async getLeaderboards(levelIds) {
            const items = await batchGet(
                levelIds.map((id) => key(`LEADERBOARD#${id}`))
            );
            return new Map(items.map((item) => [
                item.PK.slice('LEADERBOARD#'.length),
                item
            ]));
        },

        async isPublicLevel(publicId) {
            return (await get(`LEVEL#${publicId}`))?.visibility === 'public';
        },
        getLevel: (publicId) => get(`LEVEL#${publicId}`),
        getAuthorState: (authorId) => get(`AUTHOR#${authorId}`),
        getPublicationReceipt: (authorId, requestId) =>
            get(`PUBLISH#${authorId}#${requestId}`),
        getPublishedAttempt: (authorId, attemptId) =>
            get(`PUBLISHED_ATTEMPT#${authorId}#${attemptId}`),
        getPublishedDraft: (authorId, draftId) =>
            get(`PUBLISHED_DRAFT#${authorId}#${draftId}`),
        getLeaderboard: (levelId) => get(`LEADERBOARD#${levelId}`),

        async replaceLeaderboard(levelId, expectedRevision, next, publicId = null) {
            const putBoard = {
                TableName: table,
                Item: {
                    ...key(`LEADERBOARD#${levelId}`),
                    revision: next.revision,
                    entries: next.entries,
                    receipts: next.receipts,
                    ...(next.championRecording ? {
                        championRecording: next.championRecording
                    } : {}),
                    updatedAt: next.updatedAt
                },
                ConditionExpression: expectedRevision === 0
                    ? 'attribute_not_exists(PK)'
                    : '#revision = :expected',
                ...(expectedRevision === 0 ? {} : {
                    ExpressionAttributeNames: { '#revision': 'revision' },
                    ExpressionAttributeValues: { ':expected': expectedRevision }
                })
            };
            if (!publicId) {
                await client.send(new PutCommand(putBoard));
                return;
            }
            await transact(
                {
                    ConditionCheck: {
                        TableName: table,
                        Key: key(`LEVEL#${publicId}`),
                        ConditionExpression: '#visibility = :public',
                        ExpressionAttributeNames: { '#visibility': 'visibility' },
                        ExpressionAttributeValues: { ':public': 'public' }
                    }
                },
                { Put: putBoard }
            );
        },

        async attachChampionRecording(
            levelId,
            expectedRevision,
            combiId,
            attemptId,
            recording
        ) {
            const current = await get(`LEADERBOARD#${levelId}`);
            if (
                current?.championRecording?.key === recording.key &&
                current.entries?.[0]?.combiId === combiId &&
                current.entries?.[0]?.attemptId === attemptId
            ) return { idempotent: true, previous: current.championRecording };
            const previous = current?.championRecording ?? null;
            const update = {
                TableName: table,
                Key: key(`LEADERBOARD#${levelId}`),
                UpdateExpression: 'SET championRecording = :recording, #revision = #revision + :one, updatedAt = :now',
                ConditionExpression: '#revision = :expected AND #entries[0].attemptId = :attempt AND #entries[0].combiId = :combi',
                ExpressionAttributeNames: {
                    '#revision': 'revision',
                    '#entries': 'entries'
                },
                ExpressionAttributeValues: {
                    ':recording': recording,
                    ':one': 1,
                    ':now': Date.now(),
                    ':expected': expectedRevision,
                    ':attempt': attemptId,
                    ':combi': combiId
                }
            };
            if (levelId.startsWith('public:')) {
                await transact(
                    {
                        ConditionCheck: {
                            TableName: table,
                            Key: key(`LEVEL#${levelId.slice('public:'.length)}`),
                            ConditionExpression: '#visibility = :public',
                            ExpressionAttributeNames: { '#visibility': 'visibility' },
                            ExpressionAttributeValues: { ':public': 'public' }
                        }
                    },
                    { Update: update }
                );
            } else {
                await client.send(new UpdateCommand(update));
            }
            return { idempotent: false, previous };
        },

        async publishLevel({ level, board, payloadFingerprint, publicationRequestId }) {
            const authorPK = `AUTHOR#${level.authorId}`;
            const summary = {
                levelId: level.id,
                name: level.name,
                visibility: 'public',
                publishedAt: level.publishedAt
            };
            await transact(
                {
                    Update: {
                        TableName: table,
                        Key: key(authorPK),
                        UpdateExpression: 'SET activePublications = if_not_exists(activePublications, :zero) + :one',
                        ConditionExpression: 'attribute_not_exists(activePublications) OR activePublications < :max',
                        ExpressionAttributeValues: { ':zero': 0, ':one': 1, ':max': 5 }
                    }
                },
                putOnce(`LEVEL#${level.id}`, level),
                putOnce('CATALOG', {
                    levelId: level.id,
                    name: level.name,
                    authorId: level.authorId,
                    publishedAt: level.publishedAt
                }, level.catalogSk),
                putOnce(authorPK, summary, level.authorSk),
                putOnce(`LEADERBOARD#public:${level.id}`, board),
                putOnce(`PUBLISH#${level.authorId}#${publicationRequestId}`, {
                    levelId: level.id,
                    payloadFingerprint
                }),
                putOnce(`PUBLISHED_ATTEMPT#${level.authorId}#${level.firstAttemptId}`, {
                    levelId: level.id
                }),
                putOnce(`PUBLISHED_DRAFT#${level.authorId}#${level.draftId}`, {
                    levelId: level.id
                })
            );
        },

        async retireLevel(level, retiredAt) {
            const retired = {
                ':retired': 'retired',
                ':retiredAt': retiredAt,
                ':public': 'public'
            };
            await transact(
                {
                    Update: {
                        TableName: table,
                        Key: key(`LEVEL#${level.id}`),
                        UpdateExpression: 'SET #visibility = :retired, retiredAt = :retiredAt',
                        ConditionExpression: 'authorId = :authorId AND #visibility = :public',
                        ExpressionAttributeNames: { '#visibility': 'visibility' },
                        ExpressionAttributeValues: {
                            ...retired,
                            ':authorId': level.authorId
                        }
                    }
                },
                {
                    Update: {
                        TableName: table,
                        Key: key(`AUTHOR#${level.authorId}`),
                        UpdateExpression: 'ADD activePublications :minusOne',
                        ConditionExpression: 'activePublications > :zero',
                        ExpressionAttributeValues: { ':minusOne': -1, ':zero': 0 }
                    }
                },
                {
                    Delete: {
                        TableName: table,
                        Key: key('CATALOG', level.catalogSk)
                    }
                },
                {
                    Update: {
                        TableName: table,
                        Key: key(`AUTHOR#${level.authorId}`, level.authorSk),
                        UpdateExpression: 'SET #visibility = :retired, retiredAt = :retiredAt',
                        ConditionExpression: '#visibility = :public',
                        ExpressionAttributeNames: { '#visibility': 'visibility' },
                        ExpressionAttributeValues: retired
                    }
                }
            );
        },

        queryCatalog: (cursor, limit) => queryPage('CATALOG', cursor, limit),
        queryAuthorLevels: (authorId, cursor, limit) =>
            queryPage(`AUTHOR#${authorId}`, cursor, limit, 'LEVEL#'),

        async create(identity, hash, expiresAt) {
            await transact(
                putOnce(`COMBI#${identity.id}`, identity),
                putOnce(`SESSION#${hash}`, { combiId: identity.id, expiresAt })
            );
        },

        async rename(previous, next, sessionHash, now) {
            const items = [
                {
                    ConditionCheck: {
                        TableName: table,
                        Key: key(`SESSION#${sessionHash}`),
                        ConditionExpression: 'combiId = :id AND expiresAt > :now',
                        ExpressionAttributeValues: { ':id': previous.id, ':now': now }
                    }
                },
                {
                    Update: {
                        TableName: table,
                        Key: key(`COMBI#${previous.id}`),
                        UpdateExpression: 'SET #name = :name, nameKey = :key, #version = :next',
                        ConditionExpression: '#version = :previous',
                        ExpressionAttributeNames: { '#name': 'name', '#version': 'version' },
                        ExpressionAttributeValues: {
                            ':name': next.name,
                            ':key': next.nameKey,
                            ':next': next.version,
                            ':previous': previous.version
                        }
                    }
                },
                {
                    Put: {
                        TableName: table,
                        Item: { ...key(`NAME#${next.nameKey}`), combiId: previous.id },
                        ConditionExpression: 'attribute_not_exists(PK) OR combiId = :id',
                        ExpressionAttributeValues: { ':id': previous.id }
                    }
                }
            ];
            if (previous.nameKey && previous.nameKey !== next.nameKey) {
                items.push({
                    Delete: {
                        TableName: table,
                        Key: key(`NAME#${previous.nameKey}`),
                        ConditionExpression: 'combiId = :id',
                        ExpressionAttributeValues: { ':id': previous.id }
                    }
                });
            }
            await transact(...items);
        }
    };
}
