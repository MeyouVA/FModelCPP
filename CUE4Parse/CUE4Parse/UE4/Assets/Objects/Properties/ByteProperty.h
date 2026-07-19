// Ported from CUE4Parse/UE4/Assets/Objects/Properties/ByteProperty.cs
// Deliberate difference: the MAP-mode 64/16/8-bit width selection reads the Versions Options table
// ("ByteProperty.TMap64Bit" etc.), which is not ported yet, so MAP defaults to a single byte. TODO.
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class ByteProperty : public TPropertyTagType<uint8_t>
    {
    public:
        explicit ByteProperty(uint8_t value) { Value = value; }
        ByteProperty(FAssetArchive& Ar, ReadType type)
        {
            Value = type == ReadType::ZERO ? static_cast<uint8_t>(0) : Ar.Read<uint8_t>();
        }
        const char* TypeName() const override { return "ByteProperty"; }
    };
}
