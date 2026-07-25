// Ported from CUE4Parse/UE4/Wwise/Objects/AkFXParams.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkFx
    {
        uint8_t FXIndex = 0;
        uint32_t FXId = 0;
        uint8_t BitVector = 0;
        bool IsShareSet = false; // Version <= 145
        bool IsRendered = false; // Version <= 145

        AkFx() = default;

        explicit AkFx(FWwiseArchive& Ar)
        {
            if (Ar.Version <= 26)
            {
                // No additional fields for version <= 26
            }
            else if (Ar.Version <= 145)
            {
                FXIndex = Ar.Read<uint8_t>();
                FXId = Ar.Read<uint32_t>();
                IsShareSet = Ar.ReadBool();
                IsRendered = Ar.ReadBool();
            }
            else // Version > 145
            {
                FXIndex = Ar.Read<uint8_t>();
                FXId = Ar.Read<uint32_t>();
                BitVector = Ar.Read<uint8_t>();
                IsShareSet = (BitVector & (1 << 1)) != 0;
                IsRendered = (BitVector & (1 << 2)) != 0;
            }
        }
    };

    struct AkFxParams
    {
        bool BypassAll = false;
        std::vector<AkFx> Effects;

        AkFxParams() = default;

        explicit AkFxParams(FWwiseArchive& Ar)
        {
            int count;
            if (Ar.Version <= 26)
            {
                count = Ar.Read<uint32_t>() != 0 ? 1 : 0; // uNumFx (flag check for version <= 26)
            }
            else
            {
                count = Ar.Read<uint8_t>(); // uNumFx
            }

            if (count > 0)
            {
                // C# writes three switch arms here where the last two are identical; the <= 26 arm is the
                // only one that skips the bypass byte.
                if (Ar.Version > 26)
                {
                    BypassAll = Ar.ReadBool();
                }

                Effects = Ar.ReadArrayWith(count, [&Ar] { return AkFx(Ar); });
            }
        }
    };

    struct AkFxChunk
    {
        uint8_t FxIndex = 0;
        uint32_t FxId = 0;
        uint8_t IsShareSet = 0;

        AkFxChunk() = default;

        explicit AkFxChunk(FWwiseArchive& Ar)
        {
            FxIndex = Ar.Read<uint8_t>();
            FxId = Ar.Read<uint32_t>();
            IsShareSet = Ar.Read<uint8_t>();
        }

        AkFxChunk(uint8_t fxIndex, uint32_t fxId, uint8_t isShareSet)
            : FxIndex(fxIndex), FxId(fxId), IsShareSet(isShareSet) {}
    };

    class AkFxBus
    {
    public:
        uint8_t BitsFxBypass = 0;
        std::vector<AkFxChunk> FxChunks;
        std::optional<AkFxParams> FxParams; // >136
        uint32_t FxId0 = 0;
        bool IsShareSet0 = false;

        AkFxBus() = default;

        explicit AkFxBus(FWwiseArchive& Ar)
        {
            int count;

            if (Ar.Version <= 26)
            {
                const uint32_t numFX = Ar.Read<uint32_t>();
                count = numFX != 0 ? 1 : 0;
            }
            else if (Ar.Version <= 135)
            {
                count = Ar.Read<uint8_t>();
            }
            else
            {
                count = 0;
            }

            // C# branches on version 48..65 here but both arms are `count > 0`; the comment there notes
            // the environmental case only mattered for versions <= 53. Kept as one condition.
            const bool readFx = count > 0;

            if (readFx)
            {
                if (Ar.Version > 26)
                {
                    BitsFxBypass = Ar.Read<uint8_t>();
                }

                // Note this reads a *fourth*, unused byte per chunk -- it is not AkFxChunk(Ar), which
                // reads only three fields.
                FxChunks.reserve(static_cast<size_t>(count));
                for (int i = 0; i < count; i++)
                {
                    const auto fxIndex = Ar.Read<uint8_t>();
                    const auto fxId = Ar.Read<uint32_t>();
                    const auto isShareSet = Ar.Read<uint8_t>();
                    Ar.Read<uint8_t>(); // unused byte
                    FxChunks.emplace_back(fxIndex, fxId, isShareSet);
                }
            }

            if (Ar.Version > 135)
            {
                FxParams = AkFxParams(Ar);
            }

            if (Ar.Version > 89 && Ar.Version <= 145)
            {
                FxId0 = Ar.Read<uint32_t>();
                IsShareSet0 = Ar.ReadBool();
            }
        }
    };
}
