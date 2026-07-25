// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionBase.cs
#pragma once

#include "../../WwiseArchive.h"
#include "CAkActionExcept.h"
#include "CAkActionParams.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    class CAkActionBase
    {
    public:
        CAkActionParams ActionParams;
        CAkActionExcept ExceptParams;

        CAkActionBase() = default;

        explicit CAkActionBase(FWwiseArchive& Ar) : ActionParams(Ar), ExceptParams(Ar) {}
    };
}
