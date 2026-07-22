#pragma once

#include "../Parameters/KickParameters.h"

namespace gsk
{
class DualDistortionModule
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    float process (float input, const DistortionModuleParameters&, const DistortionMacros&, ClipStyle) noexcept;

    // Shared by the audio path, visual transfer editor and separation tests.
    static float shapeTransfer (float input, DistortionType type, float shape,
                                float bias, float dynamics) noexcept;

private:
    struct StageState
    {
        float toneLow {}, held {}, sag {};
        int holdCounter {};
    };

    float processStage (float, DistortionStageParameters, StageState&, const DistortionMacros&) noexcept;
    static float fold (float) noexcept;
    static float gain (float db) noexcept;

    double sampleRate { 88200.0 };
    StageState aState, bState;
    float crossoverLow {}, feedbackState {};
};
}
