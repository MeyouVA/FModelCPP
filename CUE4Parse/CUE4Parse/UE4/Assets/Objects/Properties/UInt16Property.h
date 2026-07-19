// Ported from CUE4Parse/UE4/Assets/Objects/Properties/UInt16Property.cs
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class UInt16Property : public TPropertyTagType<uint16_t>
    {
    public:
        explicit UInt16Property(uint16_t value) { Value = value; }
        UInt16Property(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0 : Ar.Read<uint16_t>(); }
        const char* TypeName() const override { return "UInt16Property"; }
    };
}
