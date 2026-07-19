// Ported from CUE4Parse/UE4/Assets/Objects/Properties/Int64Property.cs
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class Int64Property : public TPropertyTagType<int64_t>
    {
    public:
        explicit Int64Property(int64_t value) { Value = value; }
        Int64Property(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0 : Ar.Read<int64_t>(); }
        const char* TypeName() const override { return "Int64Property"; }
    };
}
