// Ported from CUE4Parse/UE4/Wwise/Plugins/IAkPluginParam.cs
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    // C# declares this as an empty marker interface, but WwisePlugin.TryParsePluginParams returns one
    // polymorphically, so the C++ version needs a virtual destructor to be owned by a smart pointer.
    class IAkPluginParam
    {
    public:
        virtual ~IAkPluginParam() = default;
    };

    // The fallback for a plugin id with no handler: keep the bytes and move on.
    class CAkDefaultParams : public IAkPluginParam
    {
    public:
        std::vector<uint8_t> PluginData;

        CAkDefaultParams(FWwiseArchive& Ar, int size) : PluginData(Ar.ReadBytes(size)) {}
    };

    // Wwise stores most gains as dB and CUE4Parse converts them to a linear multiplier on read. The
    // conversion appears dozens of times across this tree as MathF.Pow(10f, x * 0.05f); it is named here
    // so the intent is visible at each call site.
    inline float DbToLinear(float decibels) { return std::pow(10.0f, decibels * 0.05f); }
}
