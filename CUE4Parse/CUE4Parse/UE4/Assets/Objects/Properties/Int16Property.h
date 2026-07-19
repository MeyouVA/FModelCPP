// Ported from CUE4Parse/UE4/Assets/Objects/Properties/Int16Property.cs
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class Int16Property : public TPropertyTagType<int16_t>
    {
    public:
        explicit Int16Property(int16_t value) { Value = value; }
        Int16Property(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0 : Ar.Read<int16_t>(); }
        const char* TypeName() const override { return "Int16Property"; }
    };
}
