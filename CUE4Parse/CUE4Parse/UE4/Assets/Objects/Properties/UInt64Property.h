// Ported from CUE4Parse/UE4/Assets/Objects/Properties/UInt64Property.cs
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class UInt64Property : public TPropertyTagType<uint64_t>
    {
    public:
        explicit UInt64Property(uint64_t value) { Value = value; }
        UInt64Property(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0 : Ar.Read<uint64_t>(); }
        const char* TypeName() const override { return "UInt64Property"; }
    };
}
