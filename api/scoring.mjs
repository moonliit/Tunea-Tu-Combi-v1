export const RULES_VERSION = 'score-v2';

const PICKUP_POINTS = 500;
const DELIVERY_POINTS = 1000;
const PART_DELIVERY_POINTS = 3000;
const EXTRA_PARTS_TO_HALVE_SCORE = 25;
const MINIMUM_BUILD_PARTS = 2;
const SPEED_POINTS = 3000;
const SPEED_LIMIT_MS = 30000;
const MAXIMUM_COUNT = 1000000;

const roundHalfUp = (numerator, denominator) =>
    Math.floor((numerator + Math.floor(denominator / 2)) / denominator);
const ceilDivide = (numerator, denominator) =>
    Math.floor((numerator + denominator - 1) / denominator);

export function calculateScore(statistics) {
    const values = [
        statistics.initialParts,
        statistics.passengersCollected,
        statistics.passengersDelivered,
        statistics.partsDelivered,
        statistics.driverTimeMs
    ];
    if (
        !values.every(Number.isSafeInteger) ||
        statistics.initialParts <= 0 ||
        statistics.passengersCollected < 0 ||
        statistics.passengersDelivered < 0 ||
        statistics.partsDelivered < 0 ||
        statistics.driverTimeMs < 0 ||
        statistics.initialParts > MAXIMUM_COUNT ||
        statistics.passengersCollected > MAXIMUM_COUNT ||
        statistics.passengersDelivered > MAXIMUM_COUNT ||
        statistics.partsDelivered > MAXIMUM_COUNT ||
        statistics.passengersDelivered > statistics.passengersCollected ||
        statistics.partsDelivered > statistics.initialParts
    ) {
        return {
            valid: false,
            passengerPickup: 0,
            passengerDelivery: 0,
            partDelivery: 0,
            constructionPenalty: 0,
            speed: 0,
            total: 0
        };
    }

    const passengerPickup = PICKUP_POINTS * statistics.passengersCollected;
    const passengerDelivery = DELIVERY_POINTS * statistics.passengersDelivered;
    const partDelivery = roundHalfUp(
        PART_DELIVERY_POINTS * statistics.partsDelivered,
        statistics.initialParts
    );
    const speed = statistics.driverTimeMs >= SPEED_LIMIT_MS
        ? 0
        : SPEED_POINTS - ceilDivide(
            SPEED_POINTS * statistics.driverTimeMs,
            SPEED_LIMIT_MS
        );
    const earned = passengerPickup + passengerDelivery + partDelivery + speed;
    const extraParts = Math.max(0, statistics.initialParts - MINIMUM_BUILD_PARTS);
    const total = roundHalfUp(
        earned * EXTRA_PARTS_TO_HALVE_SCORE,
        EXTRA_PARTS_TO_HALVE_SCORE + extraParts
    );
    return {
        valid: true,
        passengerPickup,
        passengerDelivery,
        partDelivery,
        constructionPenalty: total - earned,
        speed,
        total
    };
}
