// Ported from CUE4Parse/UE4/Wwise/Objects/AkMusicRanSeqPlaylistItem.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct LoopInfo
    {
        int16_t Loop = 0;
        std::optional<int16_t> LoopMin;
        std::optional<int16_t> LoopMax;

        LoopInfo() = default;

        explicit LoopInfo(FWwiseArchive& Ar)
        {
            Loop = Ar.Read<int16_t>();

            if (Ar.Version > 89)
            {
                LoopMin = Ar.Read<int16_t>();
                LoopMax = Ar.Read<int16_t>();
            }
        }
    };

    struct WeightInfo
    {
        uint16_t Weight = 0;
        std::optional<uint16_t> AvoidRepeatCount;
        uint8_t IsUsingWeight = 0;
        uint8_t IsShuffle = 0;

        WeightInfo() = default;

        explicit WeightInfo(FWwiseArchive& Ar)
        {
            // The field widened to uint on the wire but C# still narrows it back to ushort.
            if (Ar.Version <= 56)
                Weight = Ar.Read<uint16_t>();
            else
                Weight = static_cast<uint16_t>(Ar.Read<uint32_t>());

            AvoidRepeatCount = Ar.Read<uint16_t>();
            IsUsingWeight = Ar.Read<uint8_t>();
            IsShuffle = Ar.Read<uint8_t>();
        }
    };

    struct AkMusicRanSeqPlaylistItem
    {
        uint32_t SegmentId = 0;
        uint32_t PlaylistItemId = 0;
        uint32_t NumChildren = 0;
        std::vector<AkMusicRanSeqPlaylistItem> Children;
        LoopInfo LoopInfoValue;
        WeightInfo WeightInfoValue;

        AkMusicRanSeqPlaylistItem() = default;

        explicit AkMusicRanSeqPlaylistItem(FWwiseArchive& Ar)
        {
            SegmentId = Ar.Read<uint32_t>();
            PlaylistItemId = Ar.Read<uint32_t>();
            NumChildren = Ar.Read<uint32_t>();

            if (Ar.Version <= 36)
            {
                if (NumChildren != 0)
                {
                    LoopInfoValue = LoopInfo(Ar);
                    WeightInfoValue = WeightInfo(Ar);
                }
                else
                {
                    // Leaf items re-read SegmentId, overwriting the one read above.
                    SegmentId = Ar.Read<uint32_t>();
                    LoopInfoValue = LoopInfo(Ar);
                    WeightInfoValue = WeightInfo(Ar);
                }
            }
            else
            {
                if (Ar.Version <= 44)
                {
                    if (NumChildren == 0)
                        SegmentId = Ar.Read<uint32_t>();
                    else
                        Ar.Read<uint32_t>(); // eRSType
                }
                else
                {
                    Ar.Read<uint32_t>(); // eRSType
                }

                LoopInfoValue = LoopInfo(Ar);
                WeightInfoValue = WeightInfo(Ar);
            }

            Children = Ar.ReadArrayWith(static_cast<int>(NumChildren),
                                        [&Ar] { return AkMusicRanSeqPlaylistItem(Ar); });
        }
    };
}
