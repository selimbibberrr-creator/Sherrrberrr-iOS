#include "DualFilterModule.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace gsk
{
namespace
{
constexpr auto pi = 3.14159265358979323846;
constexpr auto twoPi = 2.0 * pi;
float unit (float v) noexcept { return std::clamp (std::isfinite (v) ? v : 0.0f, 0.0f, 1.0f); }
float bipolar (float v) noexcept { return std::clamp (std::isfinite (v) ? v : 0.0f, -1.0f, 1.0f); }
float gain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
}

DualFilterModule::Svf::Output DualFilterModule::Svf::process (float input, float cutoff, float q, double sr) noexcept
{
    input = std::isfinite (input) ? input : 0.0f;
    cutoff = std::clamp (cutoff, 10.0f, static_cast<float> (sr * 0.42));
    q = std::clamp (q, 0.35f, 8.0f);
    const auto g = std::tan (static_cast<float> (pi) * cutoff / static_cast<float> (sr));
    const auto k = 1.0f / q;
    const auto a1 = 1.0f / (1.0f + g * (g + k));
    const auto a2 = g * a1, a3 = g * a2;
    const auto v3 = input - ic2;
    const auto v1 = a1 * ic1 + a2 * v3;
    const auto v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
    if (! std::isfinite (ic1) || ! std::isfinite (ic2)) { reset(); return {}; }
    return { v2, v1, input - k * v1 - v2 };
}

void DualFilterModule::Unit::prepare (double sr)
{
    sampleRate = std::clamp (std::isfinite (sr) ? sr : 88200.0, 8000.0, 768000.0);
    combBuffer.assign (static_cast<size_t> (std::ceil (sampleRate / 18.0)) + 8u, 0.0f);
    reset();
}

void DualFilterModule::Unit::reset() noexcept
{
    first.reset(); second.reset();
    std::fill (combBuffer.begin(), combBuffer.end(), 0.0f);
    combWrite = 0; combDampingState = 0.0f; typeFade = 1.0f;
}

float DualFilterModule::Unit::typeOutput (FilterType type, const Svf::Output& a,
                                         const Svf::Output& b, float comb) const noexcept
{
    switch (type)
    {
        case FilterType::highPass: return b.high;
        case FilterType::bandPass: return b.band * 1.4f;
        case FilterType::comb: return comb;
        case FilterType::lowPass:
        case FilterType::count:
        default: return b.low;
    }
}

float DualFilterModule::Unit::process (float input, const FilterUnitParameters& p,
                                       float frequency, float resonance, float drive) noexcept
{
    input = std::clamp (std::isfinite (input) ? input : 0.0f, -4.0f, 4.0f);
    if (! p.enabled) return input;
    frequency = std::clamp (frequency, 20.0f, static_cast<float> (sampleRate * 0.42));
    resonance = unit (resonance); drive = unit (drive);
    const auto driven = std::tanh (input * gain (18.0f * drive)) / std::max (0.15f, gain (8.0f * drive));
    const auto q = 0.55f + 7.1f * resonance * resonance;
    const auto firstOutput = first.process (driven, frequency, q, sampleRate);
    const auto firstSelected = p.type == FilterType::highPass ? firstOutput.high
                            : p.type == FilterType::bandPass ? firstOutput.band * 1.4f : firstOutput.low;
    const auto secondOutput = second.process (p.slope > 0 ? firstSelected : driven, frequency, q, sampleRate);
    const auto svfOutput = p.slope > 0 ? secondOutput : firstOutput;

    auto comb = driven;
    if (! combBuffer.empty())
    {
        const auto delay = std::clamp (sampleRate / static_cast<double> (frequency), 2.0,
                                       static_cast<double> (combBuffer.size() - 3));
        auto read = static_cast<double> (combWrite) - delay;
        while (read < 0.0) read += static_cast<double> (combBuffer.size());
        const auto index = static_cast<size_t> (read) % combBuffer.size();
        const auto next = (index + 1u) % combBuffer.size();
        const auto delayed = lerp (combBuffer[index], combBuffer[next], static_cast<float> (read - std::floor (read)));
        const auto dampCoefficient = 0.015f + 0.48f * std::pow (1.0f - unit (p.combDamping), 2.0f);
        combDampingState += (delayed - combDampingState) * dampCoefficient;
        const auto polarity = p.combPolarity > 0 ? -1.0f : 1.0f;
        const auto feedback = polarity * 0.965f * resonance * resonance;
        combBuffer[combWrite] = std::clamp (driven + std::tanh (combDampingState) * feedback, -2.0f, 2.0f);
        combWrite = (combWrite + 1u) % combBuffer.size();
        comb = std::clamp (driven * 0.55f + delayed * 0.82f, -3.0f, 3.0f);
    }

    if (p.type != targetType)
    {
        previousType = targetType; targetType = p.type; typeFade = 0.0f;
    }
    typeFade = std::min (1.0f, typeFade + static_cast<float> (1.0 / (0.006 * sampleRate)));
    const auto oldOutput = typeOutput (previousType, firstOutput, svfOutput, comb);
    const auto newOutput = typeOutput (targetType, firstOutput, svfOutput, comb);
    const auto wet = lerp (oldOutput, newOutput, typeFade);
    return std::clamp (lerp (input, wet, unit (p.mix)), -3.0f, 3.0f);
}

void DualFilterModule::prepare (double sr)
{
    sampleRate = std::clamp (std::isfinite (sr) ? sr : 88200.0, 8000.0, 768000.0);
    one.prepare (sampleRate); two.prepare (sampleRate); reset();
}

void DualFilterModule::reset() noexcept
{
    one.reset(); two.reset(); motionPhase = 0.0; feedbackState = 0.0f;
}

float DualFilterModule::modulationValue (FilterModSource source, const FilterModulationFrame& f) noexcept
{
    switch (source)
    {
        case FilterModSource::osc1: return unit (f.sourceEnvelopes[0]);
        case FilterModSource::osc2: return unit (f.sourceEnvelopes[1]);
        case FilterModSource::noise: return unit (f.sourceEnvelopes[2]);
        case FilterModSource::rumble: return unit (f.sourceEnvelopes[3]);
        case FilterModSource::sample: return unit (f.sourceEnvelopes[4]);
        case FilterModSource::pitch:
        case FilterModSource::count:
        default: return unit (f.pitchEnvelope);
    }
}

float DualFilterModule::process (float input, const FilterModuleParameters& detail,
                                 const FilterMacros& macro, const FilterModulationFrame& frame) noexcept
{
    input = std::clamp (std::isfinite (input) ? input : 0.0f, -3.0f, 3.0f);
    const auto lfo = static_cast<float> (std::sin (motionPhase));
    motionPhase += twoPi * std::clamp (static_cast<double> (detail.motionRateHz), 0.03, 24.0) / sampleRate;
    if (motionPhase >= twoPi) motionPhase -= twoPi;

    auto p1 = detail.one, p2 = detail.two;
    const auto macroCut = 4.5f * (unit (macro.cutoff) - 0.58f);
    const auto macroSweep = 5.0f * (2.0f * unit (macro.sweep) - 1.0f);
    const auto spread = 3.5f * (2.0f * unit (macro.spread) - 1.0f);
    const auto motionScale = 0.25f + 1.75f * unit (macro.motion);
    const auto linkedTwo = std::exp (lerp (std::log (std::max (20.0f, p2.frequencyHz)),
                                           std::log (std::max (20.0f, p1.frequencyHz)), unit (detail.link)));
    const auto frequency = [&] (const FilterUnitParameters& p, float base, float spreadOctaves)
    {
        const auto env = modulationValue (p.modSource, frame);
        const auto octaves = macroCut + spreadOctaves + p.envelopeAmount * macroSweep * env
                           + p.motionAmount * motionScale * lfo * 3.0f;
        return std::clamp (base * std::pow (2.0f, octaves), 20.0f, static_cast<float> (sampleRate * 0.42));
    };
    const auto f1 = frequency (p1, p1.frequencyHz, -0.5f * spread);
    const auto f2 = frequency (p2, linkedTwo, 0.5f * spread);
    const auto macroRes = 0.65f * (unit (macro.resonance) - 0.24f);
    const auto r1 = unit (p1.resonance + macroRes), r2 = unit (p2.resonance + macroRes);
    const auto macroDrive = 0.15f * unit (macro.morph);
    const auto feedback = std::tanh (feedbackState * (1.0f + 2.0f * unit (macro.feedback)))
                        * 0.72f * unit (macro.feedback) * unit (macro.feedback);
    const auto stageInput = std::clamp (input + feedback, -3.5f, 3.5f);

    float wet {};
    switch (detail.routing)
    {
        case FilterRouting::twoToOne:
            wet = one.process (two.process (stageInput, p2, f2, r2, p2.drive + macroDrive), p1, f1, r1, p1.drive + macroDrive);
            break;
        case FilterRouting::parallel:
        {
            const auto a = one.process (stageInput, p1, f1, r1, p1.drive + macroDrive);
            const auto b = two.process (stageInput, p2, f2, r2, p2.drive + macroDrive);
            const auto balance = std::clamp (0.5f + 0.5f * bipolar (detail.balance) + 0.22f * (unit (macro.route) - 0.5f), 0.0f, 1.0f);
            wet = lerp (a, b, balance);
            break;
        }
        case FilterRouting::oneToTwo:
        case FilterRouting::count:
        default:
            wet = two.process (one.process (stageInput, p1, f1, r1, p1.drive + macroDrive), p2, f2, r2, p2.drive + macroDrive);
            break;
    }
    feedbackState = std::clamp (wet, -1.5f, 1.5f);
    return std::clamp (lerp (input, wet, unit (detail.mix)), -3.0f, 3.0f);
}

float DualFilterModule::responseMagnitude (FilterType type, float f, float cutoff, float resonance,
                                           int slope, float damping, int polarity, double sr) noexcept
{
    f = std::clamp (f, 10.0f, static_cast<float> (sr * 0.49)); cutoff = std::clamp (cutoff, 10.0f, static_cast<float> (sr * 0.45));
    const auto ratio = std::max (1.0e-4f, f / cutoff);
    const auto q = 0.55f + 7.1f * unit (resonance) * unit (resonance);
    const auto denominator = std::sqrt (std::pow (1.0f - ratio * ratio, 2.0f) + ratio * ratio / (q * q));
    auto magnitude = 1.0f;
    if (type == FilterType::lowPass) magnitude = 1.0f / std::max (denominator, 1.0e-5f);
    else if (type == FilterType::highPass) magnitude = ratio * ratio / std::max (denominator, 1.0e-5f);
    else if (type == FilterType::bandPass) magnitude = ratio / std::max (q * denominator, 1.0e-5f);
    else
    {
        const auto delay = sr / cutoff;
        const auto phase = twoPi * static_cast<double> (f) * delay / sr;
        const auto fb = (polarity > 0 ? -1.0f : 1.0f) * 0.965f * unit (resonance) * unit (resonance) * (0.35f + 0.65f * (1.0f - unit (damping)));
        magnitude = static_cast<float> (std::abs (1.0 + 0.82 * std::exp (std::complex<double> (0.0, -phase)))
                   / std::max (0.05, std::abs (1.0 - static_cast<double> (fb) * std::exp (std::complex<double> (0.0, -phase)))));
    }
    if (slope > 0 && type != FilterType::comb) magnitude *= magnitude;
    return std::clamp (magnitude, 0.001f, 16.0f);
}
}
