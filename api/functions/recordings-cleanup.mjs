import { container } from '../container.mjs';

// EventBridge schedule, not an HTTP route: deletes expired recordings in S3.
// It refuses any other trigger so a stray route cannot run the sweep.
export const handler = async (event) => {
    if (event?.source !== 'aws.events')
        throw new Error('recordings-cleanup only runs from an EventBridge schedule');
    return (await container.recordings()).cleanupDue();
};
