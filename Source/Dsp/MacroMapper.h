#pragma once

#include "../Parameters/KickParameters.h"

namespace gsk
{
/** The temporary, normalised macro frame consumed by the DSP.

    XY modulation is deliberately represented only in this derived frame. It
    never changes the stored base macros, so automation and state round-trips
    remain predictable.
*/
struct MappedMacros
{
    FilterMacros filter;
    DistortionMacros distortion;
};

class MacroMapper
{
public:
    static MappedMacros map (const FilterMacros& baseFilter,
                             const DistortionMacros& baseDistortion,
                             const XYPosition& xy) noexcept;

    static MappedMacros map (const KickParameters& parameters) noexcept
    {
        return map (parameters.filter, parameters.distortion, parameters.xy);
    }
};

} // namespace gsk
