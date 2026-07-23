// Ported from CUE4Parse/UE4/Objects/Core/Math/FCapsuleShape.cs.
#pragma once

#include "FVector.h"
#include "../../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FCapsuleShape
    {
        FVector Center;       // the capsule's center point
        float Radius = 0.0f;  // the capsule's radius
        FVector Orientation;  // the capsule's orientation in space
        float Length = 0.0f;  // the capsule's length

        FCapsuleShape() = default;

        // Fields are read in declaration order, matching the C# primary constructor.
        explicit FCapsuleShape(CUE4Parse::UE4::Assets::Readers::FAssetArchive& Ar)
        {
            Center = FVector(Ar);
            Radius = Ar.ReadFReal();
            Orientation = FVector(Ar);
            Length = Ar.ReadFReal();
        }
    };
}
