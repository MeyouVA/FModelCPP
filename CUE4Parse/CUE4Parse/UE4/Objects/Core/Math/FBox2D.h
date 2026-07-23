// Ported from CUE4Parse/UE4/Objects/Core/Math/FBox2D.cs — the 2D bounding box and its generic sibling.
#pragma once

#include <string>

#include "FVector2D.h"
#include "TIntVector.h"
#include "../../../IUStruct.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FBox2D : public UE4::IUStruct
    {
        /** Holds the box's minimum point. */
        FVector2D Min;
        /** Holds the box's maximum point. */
        FVector2D Max;
        /** Holds a flag indicating whether this box is valid. */
        uint8_t bIsValid = 0;

        FBox2D() = default;
        explicit FBox2D(FArchive& Ar) : Min(Ar), Max(Ar), bIsValid(Ar.Read<uint8_t>()) {}

        std::string ToString() const
        {
            return "bIsValid=" + std::to_string(bIsValid) + ", Min=(" + Min.ToString() + "), Max=(" + Max.ToString() + ")";
        }
    };

    template <typename T>
    struct TBox2 : public UE4::IUStruct
    {
        /** Holds the box's minimum point. */
        TIntVector2<T> Min;
        /** Holds the box's maximum point. */
        TIntVector2<T> Max;
        /** Holds a flag indicating whether this box is valid. */
        uint8_t bIsValid = 0;

        TBox2() = default;
        explicit TBox2(FArchive& Ar)
            : Min(Ar.template Read<TIntVector2<T>>()), Max(Ar.template Read<TIntVector2<T>>()),
              bIsValid(Ar.Read<uint8_t>()) {}

        std::string ToString() const
        {
            return "bIsValid=" + std::to_string(bIsValid) + ", Min=(" + Min.ToString() + "), Max=(" + Max.ToString() + ")";
        }
    };
}
