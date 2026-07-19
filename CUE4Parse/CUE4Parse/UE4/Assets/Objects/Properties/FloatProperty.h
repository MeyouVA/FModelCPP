// Ported from CUE4Parse/UE4/Assets/Objects/Properties/FloatProperty.cs
#pragma once

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class FloatProperty : public TPropertyTagType<float>
    {
    public:
        explicit FloatProperty(float value) { Value = value; }
        FloatProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0.0f : Ar.Read<float>(); }
        const char* TypeName() const override { return "FloatProperty"; }
    };
}
