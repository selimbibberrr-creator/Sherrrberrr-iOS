#pragma once

#include "../Parameters/KickParameters.h"

#include <array>
#include <vector>

namespace gsk
{
struct FilterModulationFrame
{
    float pitchEnvelope { 0.0f };
    std::array<float, 5> sourceEnvelopes {};
};

class DualFilterModule
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;
    float process (float input, const FilterModuleParameters&, const FilterMacros&,
                   const FilterModulationFrame&) noexcept;

    static float responseMagnitude (FilterType, float responseFrequencyHz, float cutoffHz,
                                    float resonance, int slope, float combDamping,
                                    int combPolarity, double sampleRate) noexcept;

private:
    struct Svf
    {
        struct Output { float low {}, band {}, high {}; };
        Output process (float, float cutoff, float q, double sampleRate) noexcept;
        void reset() noexcept { ic1 = ic2 = 0.0f; }
        float ic1 {}, ic2 {};
    };

    struct Unit
    {
        void prepare (double);
        void reset() noexcept;
        float process (float, const FilterUnitParameters&, float frequencyHz,
                       float resonance, float drive) noexcept;
        float typeOutput (FilterType, const Svf::Output&, const Svf::Output&, float comb) const noexcept;

        double sampleRate { 88200.0 };
        Svf first, second;
        std::vector<float> combBuffer;
        size_t combWrite {};
        float combDampingState {};
        FilterType previousType { FilterType::lowPass }, targetType { FilterType::lowPass };
        float typeFade { 1.0f };
    };

    static float modulationValue (FilterModSource, const FilterModulationFrame&) noexcept;

    double sampleRate { 88200.0 };
    Unit one, two;
    double motionPhase {};
    float feedbackState {};
};
}
