// Ported from CUE4Parse/UE4/Wwise/Objects/AkAuxParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/Flags/EAuxParamsFlags.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::Flags::EAuxParamsFlags;

    struct AkAuxParams
    {
        bool OverrideGameAuxSends = false;
        bool UseGameAuxSends = false;
        bool OverrideUserAuxSends = false;

        EAuxParamsFlags AuxParams = EAuxParamsFlags::None;

        std::vector<uint32_t> AuxIds;
        uint32_t ReflectionsAuxBus = 0;

        AkAuxParams() = default;

        explicit AkAuxParams(FWwiseArchive& Ar)
        {
            bool hasAux;
            if (Ar.Version <= 89)
            {
                OverrideGameAuxSends = Ar.ReadBool();
                UseGameAuxSends = Ar.ReadBool();
                OverrideUserAuxSends = Ar.ReadBool();
                hasAux = Ar.ReadBool();

                // Note only two of the four bools feed the flag word; the other two stay as fields.
                if (OverrideUserAuxSends)
                    AuxParams |= EAuxParamsFlags::OverrideUserAuxSends;
                if (hasAux)
                    AuxParams |= EAuxParamsFlags::HasAux;
            }
            else
            {
                AuxParams = Ar.Read<EAuxParamsFlags>();
                hasAux = HasFlag(AuxParams, EAuxParamsFlags::HasAux);
            }

            if (hasAux)
            {
                AuxIds = Ar.ReadArray<uint32_t>(4);
            }

            if (Ar.Version > 134)
            {
                ReflectionsAuxBus = Ar.Read<uint32_t>();
            }
        }
    };
}
