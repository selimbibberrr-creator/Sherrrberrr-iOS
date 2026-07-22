#pragma once

#include "../Parameters/KickParameters.h"

#include <cstdint>

namespace gsk
{
class SourceEnvelope
{
public:
    enum class Stage { idle, attack, decay, sustain, release };

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void trigger (EnvelopeParameters parameters, bool oneShot, float oneShotLengthMs) noexcept;
    void noteOff() noexcept;
    float process() noexcept;

    bool isActive() const noexcept { return stage != Stage::idle; }
    Stage getStage() const noexcept { return stage; }
    float getCurrentValue() const noexcept { return currentValue; }

private:
    void enter (Stage next) noexcept;
    void beginRelease() noexcept;
    int64_t samplesForMs (float milliseconds) const noexcept;
    static float shape (float position, float curve) noexcept;

    double sampleRate { 44100.0 };
    EnvelopeParameters parameters;
    Stage stage { Stage::idle };
    int64_t stageSample { 0 };
    int64_t totalSample { 0 };
    int64_t oneShotGateSamples { 0 };
    bool oneShotMode { true };
    float currentValue { 0.0f };
    float releaseStart { 0.0f };
};
} // namespace gsk
