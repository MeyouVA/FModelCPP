// Ported from CUE4Parse/UE4/Objects/Core/Math/TBox3.cs — the generic 3D bounding box.
#pragma once

#include <string>

#include "TIntVector.h"
#include "../../../IUStruct.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    template <typename T>
    struct TBox3 : public UE4::IUStruct
    {
        /** Holds the box's minimum point. */
        TIntVector3<T> Min;
        /** Holds the box's maximum point. */
        TIntVector3<T> Max;
        /** Holds a flag indicating whether this box is valid. */
        uint8_t bIsValid = 0;

        TBox3() = default;
        // Members are initialised in declaration order, so Min is read before Max, as in C#.
        explicit TBox3(FArchive& Ar)
            : Min(Ar.template Read<TIntVector3<T>>()), Max(Ar.template Read<TIntVector3<T>>()),
              bIsValid(Ar.Read<uint8_t>()) {}

        std::string ToString() const
        {
            return "bIsValid=" + std::to_string(bIsValid) + ", Min=(" + Min.ToString() + "), Max=(" + Max.ToString() + ")";
        }
    };
}
