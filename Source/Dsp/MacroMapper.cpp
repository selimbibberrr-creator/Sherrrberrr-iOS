#include "MacroMapper.h"

#include <algorithm>
#include <cmath>

namespace gsk
{
namespace
{
float unit (float value, float fallback) noexcept
{
    return std::clamp (std::isfinite (value) ? value : fallback, 0.0f, 1.0f);
}

}

MappedMacros MacroMapper::map (const FilterMacros& baseFilter,
                               const DistortionMacros& baseDistortion,
                               const XYPosition& xy) noexcept
{
    MappedMacros result {
        { unit (baseFilter.cutoff, 0.58f),
          unit (baseFilter.resonance, 0.24f),
          unit (baseFilter.sweep, 0.55f),
          unit (baseFilter.morph, 0.18f),
          unit (baseFilter.spread, 0.58f),
          unit (baseFilter.route, 0.25f),
          unit (baseFilter.motion, 0.12f),
          unit (baseFilter.feedback, 0.05f) },
        { unit (baseDistortion.drive, 0.58f),
          unit (baseDistortion.heat, 0.25f),
          unit (baseDistortion.edge, 0.12f),
          unit (baseDistortion.fold, 0.0f),
          unit (baseDistortion.bias, 0.50f),
          unit (baseDistortion.crush, 0.0f),
          unit (baseDistortion.bite, 0.30f),
          unit (baseDistortion.mix, 0.82f) }
    };

    // XY is now applied by the processor's persisted, data-driven mapping matrix.
    // Keeping this mapper base-only guarantees that a custom target is never
    // affected a second time by a hidden hard-coded route.
    juce::ignoreUnused (xy);
    return result;
}

} // namespace gsk
