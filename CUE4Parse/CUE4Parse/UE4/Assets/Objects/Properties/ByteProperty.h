// Ported from CUE4Parse/UE4/Assets/Objects/Properties/ByteProperty.cs
// The MAP-mode width is selected by the Versions Options table: a handful of games store a TMap's byte keys
// widened to 8/16/64 bits. All three options default to false, so MAP reads a single byte unless a provider
// overrides them.
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
            if (type == ReadType::ZERO) Value = 0;
            else if (type == ReadType::MAP && Ar.Versions["ByteProperty.TMap64Bit"])
                Value = static_cast<uint8_t>(Ar.Read<uint64_t>());
            else if (type == ReadType::MAP && Ar.Versions["ByteProperty.TMap16Bit"])
                Value = static_cast<uint8_t>(Ar.Read<uint16_t>());
            else if (type == ReadType::MAP && Ar.Versions["ByteProperty.TMap8Bit"])
                Value = Ar.Read<uint8_t>();
            else Value = Ar.Read<uint8_t>();
        }
        const char* TypeName() const override { return "ByteProperty"; }
    };
}
