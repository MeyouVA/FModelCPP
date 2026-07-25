// Ported from CUE4Parse/UE4/Wwise/Plugins/PolyspectralMBC/CMBCRuntimeParams.cs
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::PolyspectralMBC
{
    enum class MBCSidechainMode : int32_t
    {
        Off = 0x0,
        ReadSingle = 0x1,
        WriteSingle = 0x2,
        ReadMultiple = 0x3,
        WriteMultiple = 0x4
    };

    struct MBCBandParams
    {
        float Threshold = 0;
        float Ratio = 0;
        float Gain = 0;
        float Attack = 0;
        float Release = 0;
        // Four bytes each on the wire, despite being flags.
        bool Solo = false;
        bool Bypass = false;

        MBCBandParams() = default;

        explicit MBCBandParams(FWwiseArchive& Ar)
        {
            Threshold = Ar.Read<float>();
            Ratio = Ar.Read<float>();
            Gain = Ar.Read<float>();
            Attack = Ar.Read<float>();
            Release = Ar.Read<float>();
            Solo = Ar.Read<int32_t>() != 0;
            Bypass = Ar.Read<int32_t>() != 0;
        }
    };

    // This plugin has no version field of its own: instead a -42.0f sentinel in the first float signals
    // "a mode number follows", and that mode gates each later block. Wwise's own bank version is unused.
    class CMBCRuntimeParams : public IAkPluginParam
    {
    public:
        float PercentWet = 0;
        float InputGain = 0;
        float OutputGain = 0;
        int32_t NumBands = 0;
        std::vector<float> Crossover;
        std::vector<MBCBandParams> Bands;
        bool BypassCenter = false;
        bool BypassLFE = false;
        bool BypassOther = false;
        bool ClipOutput = false;
        MBCSidechainMode SideChainMode = static_cast<MBCSidechainMode>(0);
        std::string SidechainOutputName;

        CMBCRuntimeParams(FWwiseArchive& Ar, int size)
        {
            const int64_t start = Ar.Position;
            const float value = Ar.Read<float>();
            int mode = 0;
            if (value != -42.0f)
            {
                PercentWet = value;
            }
            else
            {
                mode = Ar.Read<int32_t>();
                PercentWet = mode >= 1 ? Ar.Read<float>() : 0;
            }
            InputGain = Ar.Read<float>();
            OutputGain = Ar.Read<float>();
            NumBands = Ar.Read<int32_t>();

            Crossover = Ar.ReadArray<float>(3);
            Bands = Ar.ReadArrayWith(NumBands, [&Ar] { return MBCBandParams(Ar); });

            if (mode >= 1)
            {
                BypassCenter = Ar.Read<int32_t>() != 0;
                BypassLFE = Ar.Read<int32_t>() != 0;
                BypassOther = Ar.Read<int32_t>() != 0;
            }

            if (mode >= 2) ClipOutput = Ar.Read<int32_t>() != 0;
            if (mode >= 3) SideChainMode = Ar.Read<MBCSidechainMode>();

            if (mode >= 4)
            {
                // Unterminated-name guard: read at most what is left of the section, capped at 64 bytes,
                // then rewind to just past the NUL. C#'s IndexOf returns -1 when absent, so ind is 0 and
                // the position rewinds exactly to `end`.
                const int64_t end = Ar.Position;
                const int strSize = std::clamp(static_cast<int>(size - (end - start)), 0, 64);
                auto temp = Ar.ReadArray<uint8_t>(strSize);
                const auto nul = std::find(temp.begin(), temp.end(), static_cast<uint8_t>(0));
                const int ind = nul == temp.end() ? 0 : static_cast<int>(nul - temp.begin()) + 1;
                SidechainOutputName.assign(reinterpret_cast<const char*>(temp.data()), static_cast<size_t>(ind));
                Ar.Position = end + ind;
            }
        }
    };
}
