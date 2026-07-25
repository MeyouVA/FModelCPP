// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionExcept.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    struct WwiseObjectIDext
    {
        uint32_t Id = 0;
        bool IsBus = false;

        WwiseObjectIDext() = default;

        explicit WwiseObjectIDext(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();

            if (Ar.Version > 65)
            {
                IsBus = Ar.ReadBool();
            }
        }
    };

    class CAkActionExcept
    {
    public:
        std::vector<WwiseObjectIDext> ExceptionElements;

        CAkActionExcept() = default;

        // CAkActionExcept::SetExceptParams
        explicit CAkActionExcept(FWwiseArchive& Ar)
        {
            int exceptionListSize;
            if (Ar.Version <= 122)
            {
                // Four bytes on the wire, but C# truncates to a byte -- kept.
                exceptionListSize = static_cast<uint8_t>(Ar.Read<uint32_t>());
            }
            else
            {
                exceptionListSize = Ar.Read7BitEncodedIntBE();
            }

            ExceptionElements = Ar.ReadArrayWith(exceptionListSize, [&Ar] { return WwiseObjectIDext(Ar); });
        }
    };
}
