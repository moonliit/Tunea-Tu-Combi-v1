import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import {
    BatchGetCommand,
    DynamoDBDocumentClient,
    GetCommand,
    PutCommand,
    TransactWriteCommand
} from '@aws-sdk/lib-dynamodb';

// only sessions use expiresAt (TTL), data belongs to COMBI#<id>
export function createStore(table, client = DynamoDBDocumentClient.from(new DynamoDBClient({}))) {
    const key = (PK) => ({ PK, SK: 'META' });
    const put = (PK, item) => ({
        Put: {
            TableName: table,
            Item: { ...key(PK), ...item },
            ConditionExpression: 'attribute_not_exists(PK)'
        }
    });
    return {
        async get(PK) {
            return (
                await client.send(
                    new GetCommand({ TableName: table, Key: key(PK), ConsistentRead: true })
                )
            ).Item;
        },
        async getIdentities(ids) {
            if (!ids.length) return new Map();
            let keys = ids.map((id) => key(`COMBI#${id}`));
            const items = [];
            for (let attempt = 0; keys.length && attempt < 4; attempt++) {
                const result = await client.send(new BatchGetCommand({
                    RequestItems: {
                        [table]: {
                            Keys: keys,
                            ConsistentRead: true,
                            ProjectionExpression: 'PK, id, #name',
                            ExpressionAttributeNames: { '#name': 'name' }
                        }
                    }
                }));
                items.push(...(result.Responses?.[table] ?? []));
                keys = result.UnprocessedKeys?.[table]?.Keys ?? [];
            }
            return new Map(
                items.map((item) => [item.id, item])
            );
        },
        async isPublicLevel(publicId) {
            const item = await this.get(`LEVEL#${publicId}`);
            return item?.visibility === 'public';
        },
        async getLeaderboard(levelId) {
            return this.get(`LEADERBOARD#${levelId}`);
        },
        async replaceLeaderboard(levelId, expectedRevision, next) {
            const item = {
                ...key(`LEADERBOARD#${levelId}`),
                revision: next.revision,
                entries: next.entries,
                receipts: next.receipts,
                updatedAt: next.updatedAt
            };
            await client.send(new PutCommand({
                TableName: table,
                Item: item,
                ConditionExpression: expectedRevision === 0
                    ? 'attribute_not_exists(PK)'
                    : '#revision = :expected',
                ...(expectedRevision === 0 ? {} : {
                    ExpressionAttributeNames: { '#revision': 'revision' },
                    ExpressionAttributeValues: { ':expected': expectedRevision }
                })
            }));
        },
        async create(identity, hash, expiresAt) {
            await client.send(
                new TransactWriteCommand({
                    TransactItems: [
                        put(`COMBI#${identity.id}`, identity),
                        put(`SESSION#${hash}`, { combiId: identity.id, expiresAt })
                    ]
                })
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
            if (previous.nameKey && previous.nameKey !== next.nameKey)
                items.push({
                    Delete: {
                        TableName: table,
                        Key: key(`NAME#${previous.nameKey}`),
                        ConditionExpression: 'combiId = :id',
                        ExpressionAttributeValues: { ':id': previous.id }
                    }
                });
            await client.send(new TransactWriteCommand({ TransactItems: items }));
        }
    };
}
