// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionPlay.cs
#pragma once

#include <cstdint>
#include <optional>

#include "../../WwiseArchive.h"
#include "../../Enums/EAkBankTypeEnum.h"
#include "CAkActionParams.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    using CUE4Parse::UE4::Wwise::Enums::EAkBankTypeEnum;

    class CAkActionPlay
    {
    public:
        CAkActionParams ActionParams;
        std::optional<uint32_t> BankId;
        EAkBankTypeEnum BankType = static_cast<EAkBankTypeEnum>(0);

        CAkActionPlay() = default;

        // CAkActionPlay::SetActionParams
        explicit CAkActionPlay(FWwiseArchive& Ar)
        {
            ActionParams = CAkActionParams(Ar);
            if (Ar.Version > 26)
            {
                BankId = Ar.Read<uint32_t>();
            }

            if (Ar.Version >= 144)
            {
                BankType = Ar.Read<EAkBankTypeEnum>();
            }
        }
    };
}
