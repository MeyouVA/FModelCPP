// Ported from CUE4Parse/UE4/Assets/Objects/Properties/UInt32Property.cs
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class UInt32Property : public TPropertyTagType<uint32_t>
    {
    public:
        explicit UInt32Property(uint32_t value) { Value = value; }
        UInt32Property(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0 : Ar.Read<uint32_t>(); }
        const char* TypeName() const override { return "UInt32Property"; }
    };
}
