#include "DualDistortionModule.h"

#include <algorithm>
#include <cmath>

namespace gsk
{
namespace
{
constexpr auto twoPi = 6.28318530717958647692;
float unit (float v) noexcept { return std::clamp (std::isfinite (v) ? v : 0.0f, 0.0f, 1.0f); }
float bipolar (float v) noexcept { return std::clamp (std::isfinite (v) ? v : 0.0f, -1.0f, 1.0f); }
float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
}

float DualDistortionModule::gain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

float DualDistortionModule::fold (float input) noexcept
{
    auto wrapped = std::fmod (input + 1.0f, 4.0f);
    if (wrapped < 0.0f) wrapped += 4.0f;
    return wrapped <= 2.0f ? wrapped - 1.0f : 3.0f - wrapped;
}

float DualDistortionModule::shapeTransfer (float input, DistortionType type, float shape,
                                           float bias, float dynamics) noexcept
{
    input = std::clamp (std::isfinite (input) ? input : 0.0f, -16.0f, 16.0f);
    shape = unit (shape); bias = bipolar (bias); dynamics = unit (dynamics);
    const auto x = input + bias * (0.18f + 0.38f * shape);
    const auto compensate = [&] (auto fn) { return fn (x) - fn (bias * (0.18f + 0.38f * shape)); };
    auto y = 0.0f;
    switch (type)
    {
        case DistortionType::hardClip:
        {
            const auto threshold = 1.0f - 0.82f * shape;
            y = std::clamp (x, -threshold, threshold) / std::max (0.08f, threshold);
            break;
        }
        case DistortionType::tube:
            y = compensate ([&] (float v) { const auto even = v * v * (0.08f + 0.22f * shape); return std::tanh (v + even) / (1.0f + 0.12f * std::abs (v)); });
            break;
        case DistortionType::tape:
            y = compensate ([&] (float v) { return std::atan (v * (1.4f + 3.5f * shape)) * (2.0f / 3.14159265f) + 0.08f * std::sin (v * 2.0f); });
            break;
        case DistortionType::diode:
            y = compensate ([&] (float v) { const auto positive = 1.0f - std::exp (-std::max (0.0f, v) * (2.0f + 7.0f * shape)); const auto negative = -0.55f * (1.0f - std::exp (-std::max (0.0f, -v) * (1.3f + 3.0f * shape))); return positive + negative; });
            break;
        case DistortionType::asymmetric:
            y = x >= 0.0f ? std::tanh (x * (1.5f + 6.0f * shape))
                          : 0.72f * std::tanh (x * (0.8f + 2.2f * dynamics));
            break;
        case DistortionType::fuzz:
        {
            const auto gate = 0.02f + 0.18f * (1.0f - dynamics);
            y = std::abs (x) < gate ? 0.0f : std::copysign (1.0f - std::exp (-std::abs (x) * (4.0f + 18.0f * shape)), x);
            break;
        }
        case DistortionType::wavefold:
            y = fold (x * (1.0f + 8.0f * shape));
            break;
        case DistortionType::sineFold:
            y = std::sin (x * (1.0f + 10.0f * shape)) * (0.72f + 0.28f * std::cos (x * dynamics));
            break;
        case DistortionType::rectify:
        {
            const auto full = std::abs (x) - std::abs (bias * 0.25f);
            const auto half = std::max (0.0f, x) - std::max (0.0f, bias * 0.25f);
            y = lerp (half, full * 2.0f - 0.5f, shape);
            break;
        }
        case DistortionType::bitcrush:
        {
            const auto bits = 16.0f - 13.0f * shape;
            const auto levels = std::pow (2.0f, bits);
            y = std::round (std::clamp (x, -1.0f, 1.0f) * levels) / levels;
            break;
        }
        case DistortionType::sampleCrush:
        {
            const auto levels = std::pow (2.0f, 12.0f - 8.0f * dynamics);
            y = std::round (std::tanh (x * (1.0f + 4.0f * shape)) * levels) / levels;
            break;
        }
        case DistortionType::softClip:
        case DistortionType::count:
        default:
        {
            const auto amount = 1.0f + 5.0f * shape;
            y = compensate ([&] (float v) { return std::tanh (v * amount) / std::tanh (amount); });
            break;
        }
    }
    return std::clamp (std::isfinite (y) ? y : 0.0f, -1.4f, 1.4f);
}

void DualDistortionModule::prepare (double sr) noexcept
{
    sampleRate = std::clamp (std::isfinite (sr) ? sr : 88200.0, 8000.0, 768000.0);
    reset();
}

void DualDistortionModule::reset() noexcept
{
    aState = {}; bState = {}; crossoverLow = feedbackState = 0.0f;
}

float DualDistortionModule::processStage (float input, DistortionStageParameters p,
                                          StageState& state, const DistortionMacros& macro) noexcept
{
    if (! p.enabled) return input;
    p.driveDb = std::clamp (p.driveDb + 24.0f * (unit (macro.drive) - 0.58f), 0.0f, 42.0f);
    p.shape = unit (p.shape + 0.32f * unit (macro.edge) + 0.24f * unit (macro.fold));
    p.bias = bipolar (p.bias + 0.30f * (2.0f * unit (macro.bias) - 1.0f));
    p.dynamics = unit (p.dynamics + 0.25f * (unit (macro.heat) - 0.25f));

    const auto toneCutoff = 300.0f * std::pow (40.0f, 0.5f + 0.5f * bipolar (p.tone));
    const auto coefficient = 1.0f - std::exp (-static_cast<float> (twoPi) * toneCutoff / static_cast<float> (sampleRate));
    state.toneLow += (input - state.toneLow) * coefficient;
    const auto tilted = input + (input - state.toneLow) * 1.8f * std::max (0.0f, p.tone)
                              - state.toneLow * 0.65f * std::max (0.0f, -p.tone);
    const auto driven = std::clamp (tilted * gain (p.driveDb), -16.0f, 16.0f);
    auto wet = shapeTransfer (driven, p.type, p.shape, p.bias, p.dynamics);

    if (p.type == DistortionType::sampleCrush)
    {
        const auto hold = 1 + static_cast<int> (std::round (31.0f * p.shape * p.shape));
        if (state.holdCounter <= 0) { state.held = wet; state.holdCounter = hold; }
        else --state.holdCounter;
        wet = state.held;
    }
    if (p.type == DistortionType::tube || p.type == DistortionType::tape)
    {
        const auto sagRate = static_cast<float> (1.0 - std::exp (-1.0 / (sampleRate * (0.004 + 0.08 * p.dynamics))));
        state.sag += (std::abs (wet) - state.sag) * sagRate;
        wet *= 1.0f - 0.32f * p.dynamics * state.sag;
    }
    const auto autoGain = gain (-0.34f * p.driveDb - 2.0f * p.shape);
    wet = std::clamp (wet * autoGain * gain (p.trimDb), -1.5f, 1.5f);
    return std::clamp (lerp (input, wet, unit (p.mix)), -1.6f, 1.6f);
}

float DualDistortionModule::process (float input, const DistortionModuleParameters& detail,
                                     const DistortionMacros& macro, ClipStyle clipStyle) noexcept
{
    input = std::clamp (std::isfinite (input) ? input : 0.0f, -4.0f, 4.0f);
    const auto feedback = std::tanh (feedbackState * (1.0f + 2.2f * unit (detail.feedback)))
                        * 0.58f * unit (detail.feedback) * unit (detail.feedback);
    const auto stageInput = std::clamp (input + feedback, -5.0f, 5.0f);
    float wet {};
    switch (detail.routing)
    {
        case DistortionRouting::bToA:
            wet = processStage (processStage (stageInput, detail.b, bState, macro), detail.a, aState, macro);
            break;
        case DistortionRouting::parallel:
        {
            const auto a = processStage (stageInput, detail.a, aState, macro);
            const auto b = processStage (stageInput, detail.b, bState, macro);
            wet = lerp (a, b, std::clamp (0.5f + 0.5f * bipolar (detail.balance), 0.0f, 1.0f));
            break;
        }
        case DistortionRouting::bandSplit:
        {
            const auto cutoff = std::clamp (detail.crossoverHz, 40.0f, static_cast<float> (sampleRate * 0.35));
            const auto coefficient = 1.0f - std::exp (-static_cast<float> (twoPi) * cutoff / static_cast<float> (sampleRate));
            crossoverLow += (stageInput - crossoverLow) * coefficient;
            const auto low = processStage (crossoverLow, detail.a, aState, macro);
            const auto high = processStage (stageInput - crossoverLow, detail.b, bState, macro);
            wet = low + high;
            break;
        }
        case DistortionRouting::aToB:
        case DistortionRouting::count:
        default:
            wet = processStage (processStage (stageInput, detail.a, aState, macro), detail.b, bState, macro);
            break;
    }
    feedbackState = std::clamp (wet, -1.3f, 1.3f);
    switch (clipStyle)
    {
        case ClipStyle::hard: wet = std::clamp (wet * 1.08f, -1.0f, 1.0f); break;
        case ClipStyle::fold: wet = fold (wet * 1.12f); break;
        case ClipStyle::crush: wet = std::round (wet * 128.0f) / 128.0f; break;
        case ClipStyle::warm:
        case ClipStyle::count:
        default: wet = std::tanh (wet * 1.12f); break;
    }
    return std::clamp (lerp (input, wet, unit (macro.mix)), -1.45f, 1.45f);
}
}
