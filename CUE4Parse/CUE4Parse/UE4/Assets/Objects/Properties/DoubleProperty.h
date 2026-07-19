// Ported from CUE4Parse/UE4/Assets/Objects/Properties/DoubleProperty.cs
#pragma once

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class DoubleProperty : public TPropertyTagType<double>
    {
    public:
        explicit DoubleProperty(double value) { Value = value; }
        DoubleProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0.0 : Ar.Read<double>(); }
        const char* TypeName() const override { return "DoubleProperty"; }
    };
}
