#include "SourceEnvelope.h"

#include <algorithm>
#include <cmath>

namespace gsk
{
void SourceEnvelope::prepare (double newSampleRate) noexcept
{
    sampleRate = std::clamp (std::isfinite (newSampleRate) ? newSampleRate : 44100.0, 8000.0, 768000.0);
    reset();
}

void SourceEnvelope::reset() noexcept
{
    stage = Stage::idle;
    stageSample = totalSample = 0;
    currentValue = releaseStart = 0.0f;
}

void SourceEnvelope::trigger (EnvelopeParameters next, bool oneShot, float oneShotLengthMs) noexcept
{
    parameters.attackMs = std::clamp (std::isfinite (next.attackMs) ? next.attackMs : 0.0f, 0.0f, 300.0f);
    parameters.decayMs = std::clamp (std::isfinite (next.decayMs) ? next.decayMs : 90.0f, 1.0f, 3000.0f);
    parameters.sustain = std::clamp (std::isfinite (next.sustain) ? next.sustain : 0.65f, 0.0f, 1.0f);
    parameters.releaseMs = std::clamp (std::isfinite (next.releaseMs) ? next.releaseMs : 420.0f, 1.0f, 3000.0f);
    parameters.curve = std::clamp (std::isfinite (next.curve) ? next.curve : 0.0f, -1.0f, 1.0f);
    oneShotMode = oneShot;
    oneShotGateSamples = samplesForMs (std::clamp (oneShotLengthMs, 1.0f, 3000.0f));
    totalSample = 0;
    currentValue = 0.0f;
    enter (parameters.attackMs <= 0.0001f ? Stage::decay : Stage::attack);
}

void SourceEnvelope::noteOff() noexcept
{
    if (! oneShotMode)
        beginRelease();
}

float SourceEnvelope::process() noexcept
{
    if (stage == Stage::idle)
        return 0.0f;

    if (oneShotMode && totalSample >= oneShotGateSamples && stage != Stage::release)
        beginRelease();

    switch (stage)
    {
        case Stage::attack:
        {
            const auto duration = std::max<int64_t> (1, samplesForMs (parameters.attackMs));
            const auto position = std::min (1.0f, static_cast<float> (stageSample + 1) / static_cast<float> (duration));
            currentValue = shape (position, parameters.curve);
            if (++stageSample >= duration) { currentValue = 1.0f; enter (Stage::decay); }
            break;
        }
        case Stage::decay:
        {
            const auto duration = std::max<int64_t> (1, samplesForMs (parameters.decayMs));
            const auto position = std::min (1.0f, static_cast<float> (stageSample + 1) / static_cast<float> (duration));
            currentValue = 1.0f + (parameters.sustain - 1.0f) * shape (position, parameters.curve);
            if (++stageSample >= duration) { currentValue = parameters.sustain; enter (Stage::sustain); }
            break;
        }
        case Stage::sustain:
            currentValue = parameters.sustain;
            ++stageSample;
            break;
        case Stage::release:
        {
            const auto duration = std::max<int64_t> (1, samplesForMs (parameters.releaseMs));
            const auto position = std::min (1.0f, static_cast<float> (stageSample + 1) / static_cast<float> (duration));
            currentValue = releaseStart * (1.0f - shape (position, parameters.curve));
            if (++stageSample >= duration || currentValue <= 1.0e-6f) reset();
            break;
        }
        case Stage::idle:
        default:
            currentValue = 0.0f;
            break;
    }

    ++totalSample;
    return std::clamp (std::isfinite (currentValue) ? currentValue : 0.0f, 0.0f, 1.0f);
}

void SourceEnvelope::enter (Stage next) noexcept
{
    stage = next;
    stageSample = 0;
}

void SourceEnvelope::beginRelease() noexcept
{
    if (stage == Stage::idle || stage == Stage::release)
        return;
    releaseStart = std::clamp (currentValue, 0.0f, 1.0f);
    enter (Stage::release);
}

int64_t SourceEnvelope::samplesForMs (float milliseconds) const noexcept
{
    return std::max<int64_t> (0, static_cast<int64_t> (std::llround (milliseconds * 0.001 * sampleRate)));
}

float SourceEnvelope::shape (float position, float curve) noexcept
{
    position = std::clamp (position, 0.0f, 1.0f);
    curve = std::clamp (curve, -1.0f, 1.0f);
    const auto exponent = 1.0f + 3.0f * std::abs (curve);
    return curve >= 0.0f ? std::pow (position, exponent)
                         : 1.0f - std::pow (1.0f - position, exponent);
}
} // namespace gsk
