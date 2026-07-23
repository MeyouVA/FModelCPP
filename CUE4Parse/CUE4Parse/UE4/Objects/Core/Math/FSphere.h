// Ported from CUE4Parse/UE4/Objects/Core/Math/FSphere.cs — the bounding sphere.
#pragma once

#include "FVector.h"
#include "TIntVector.h"
#include "../../../IUStruct.h"
#include "../../../Readers/FArchive.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FSphere : public UE4::IUStruct
    {
        /** The sphere's center point. */
        FVector Center;
        /** The sphere's radius. */
        float W = 0.0f;

        FSphere() = default;
        FSphere(float x, float y, float z, float w) : Center(x, y, z), W(w) {}
        FSphere(FVector center, float w) : Center(center), W(w) {}
        FSphere(const TIntVector3<float>& center, float w) : Center(center.X, center.Y, center.Z), W(w) {}
        FSphere(const TIntVector3<double>& center, double w)
            : Center(static_cast<float>(center.X), static_cast<float>(center.Y), static_cast<float>(center.Z)),
              W(static_cast<float>(w)) {}

        explicit FSphere(FArchive& Ar) : Center(Ar)
        {
            if (Ar.Ver() > Versions::EUnrealEngineObjectUE3Version::Release61) W = Ar.ReadFReal();
        }

        friend FSphere operator*(const FSphere& a, float scale) { return FSphere(a.Center * scale, a.W * scale); }
    };
}
