// Ported from CUE4Parse/UE4/Wwise/Objects/CAkEnvironmentsMgr.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../WwiseArchive.h"
#include "AkConversionTable.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    class CAkEnvironmentsMgr
    {
    public:
        uint32_t AttenuationId = 0;
        // C#'s CAkConversionTable[,] -- a rectangular array, kept as row-major rows here.
        std::optional<std::vector<std::vector<CAkConversionTable>>> ConversionTableEntries;

        CAkEnvironmentsMgr() = default;

        // CAkBankMgr::ProcessEnvSettingsChunk
        explicit CAkEnvironmentsMgr(FWwiseArchive& Ar)
        {
            if (Ar.Version > 154)
            {
                AttenuationId = Ar.Read<uint32_t>();
                return; // Yes, that's it
            }

            int maxX, maxY;
            if (Ar.Version <= 89)       { maxX = 2; maxY = 2; }
            else if (Ar.Version <= 150) { maxX = 2; maxY = 3; }
            else                        { maxX = 4; maxY = 3; }

            // Faithful quirk: C# builds this table into a local and never assigns it to the field, so
            // ConversionTableEntries stays null even though the bytes are consumed. The reads are what
            // matter -- the archive has to end up in the right place -- so the same shape is kept here.
            std::vector<std::vector<CAkConversionTable>> conversionTable(maxX);
            for (int i = 0; i < maxX; i++)
            {
                conversionTable[i].reserve(static_cast<size_t>(maxY));
                for (int j = 0; j < maxY; j++)
                {
                    Ar.Read<uint8_t>(); // CurveEnabled
                    conversionTable[i].emplace_back(Ar);
                }
            }
        }
    };
}
