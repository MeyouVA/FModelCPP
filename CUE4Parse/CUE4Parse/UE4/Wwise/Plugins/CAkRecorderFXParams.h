// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkRecorderFXParams.cs
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkRecorderFXParams
    {
#pragma pack(push, 4)
        struct AkRecorderRTPCParams
        {
            float Center;
            float Front;
            float Surround;
            float Rear;
            float LFE;
        };
#pragma pack(pop)

        struct AkRecorderNonRTPCParams
        {
            int16_t Format = 0;
            std::string Filename;
            bool DownmixToStereo = false;
            bool ApplyDownstreamVolume = false;
            int16_t AmbisonicsChannelOrdering = 0;

            static constexpr int _MaxCount = 0x103;

            AkRecorderNonRTPCParams() = default;

            // The filename has no length prefix: the remaining section size bounds the search, and the
            // real length is found by scanning for the NUL. The archive is rewound and re-read so the
            // cursor lands exactly past the terminator.
            AkRecorderNonRTPCParams(FWwiseArchive& Ar, int size)
            {
                Format = Ar.Read<int16_t>();
                int len = std::min(size - 2, _MaxCount);
                const int64_t saved = Ar.Position;
                auto data = Ar.ReadArray<uint8_t>(len);
                const auto nul = std::find(data.begin(), data.end(), static_cast<uint8_t>(0));
                // C#'s Array.IndexOf returns -1 when absent, so len becomes 0 and the name is empty.
                len = nul == data.end() ? 0 : static_cast<int>(nul - data.begin()) + 1;
                Ar.Position = saved;
                if (len > 0)
                {
                    auto bytes = Ar.ReadBytes(len);
                    Filename.assign(reinterpret_cast<const char*>(bytes.data()), static_cast<size_t>(len - 1));
                }
                DownmixToStereo = Ar.Read<uint8_t>() != 0;
                ApplyDownstreamVolume = Ar.Read<uint8_t>() != 0;
                if (Ar.Version >= 134)
                    AmbisonicsChannelOrdering = Ar.Read<int16_t>();
            }
        };

        AkRecorderRTPCParams RTPC{};
        AkRecorderNonRTPCParams NonRTPC;

        AkRecorderFXParams() = default;

        // 20 is sizeof(AkRecorderRTPCParams); the rest of the section belongs to the non-RTPC block.
        AkRecorderFXParams(FWwiseArchive& Ar, int size)
            : RTPC(Ar.Read<AkRecorderRTPCParams>()), NonRTPC(Ar, size - 20) {}
    };

    class CAkRecorderFXParams : public IAkPluginParam
    {
    public:
        AkRecorderFXParams Params;

        CAkRecorderFXParams(FWwiseArchive& Ar, int size) : Params(Ar, size) {}
    };
}
