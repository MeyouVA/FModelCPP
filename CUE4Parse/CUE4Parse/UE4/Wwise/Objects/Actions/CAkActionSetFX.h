// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionSetFX.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "CAkActionExcept.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    class CAkActionSetFX
    {
    public:
        bool IsAudioDeviceElement = false;
        uint8_t SlotIndex = 0;
        uint32_t FxId = 0;
        bool IsShared = false;
        CAkActionExcept ExceptParams;

        CAkActionSetFX() = default;

        // CAkActionSetFX::SetActionParams
        explicit CAkActionSetFX(FWwiseArchive& Ar)
        {
            IsAudioDeviceElement = Ar.ReadBool();
            SlotIndex = Ar.Read<uint8_t>();
            FxId = Ar.Read<uint32_t>();
            IsShared = Ar.ReadBool();
            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
