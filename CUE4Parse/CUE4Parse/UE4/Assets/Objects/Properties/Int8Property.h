// Ported from CUE4Parse/UE4/Assets/Objects/Properties/Int8Property.cs
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class Int8Property : public TPropertyTagType<int8_t>
    {
    public:
        explicit Int8Property(int8_t value) { Value = value; }
        Int8Property(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0 : Ar.Read<int8_t>(); }
        const char* TypeName() const override { return "Int8Property"; }
    };
}
