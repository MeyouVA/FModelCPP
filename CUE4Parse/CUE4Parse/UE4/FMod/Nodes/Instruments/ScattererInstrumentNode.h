// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/ScattererInstrumentNode.cs
#pragma once

#include <limits>
#include <memory>
#include <optional>

#include "BaseInstrumentNode.h"
#include "../PlaylistNode.h"
#include "../../Objects/FModGuid.h"
#include "../../Objects/FRangeFloat.h"
#include "../../Objects/FQuantization.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class ScattererInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        int32_t MaximumSpawnPolyphony = 0;
        int32_t SpawnCount = 0;
        Objects::FRangeFloat SpawnTime;
        int32_t SpawnPolyphonyLimitBehavior = 0;
        float SpawnRate = 0.0f;
        std::optional<Objects::FQuantization> SpawnQuantization;
        std::unique_ptr<PlaylistNode> PlaylistBody;

        explicit ScattererInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            MaximumSpawnPolyphony = Ar.Read<int32_t>();
            SpawnCount = Ar.Read<int32_t>();

            if (FModReader::Version() >= 0x8a)
            {
                if (FModReader::Version() < 0x8e && SpawnCount == std::numeric_limits<int32_t>::max())
                    SpawnCount = 0x21;
            }
            else if (SpawnCount == 0 || SpawnCount == std::numeric_limits<int32_t>::max())
            {
                SpawnCount = 0x21;
            }

            SpawnTime = Objects::FRangeFloat(Ar);

            if (FModReader::Version() < 0x68)
            {
                (void) Ar.Read<float>(); // Legacy value 1
                (void) Ar.Read<float>(); // Legacy value 2
            }

            if (FModReader::Version() >= 0x39)
                SpawnPolyphonyLimitBehavior = Ar.Read<int32_t>();

            if (FModReader::Version() >= 0x5e)
                SpawnRate = Ar.Read<float>();

            if (FModReader::Version() >= 0x85)
                SpawnQuantization = Objects::FQuantization(Ar);
        }
    };
}
