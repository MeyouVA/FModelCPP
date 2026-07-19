// Ported from CUE4Parse/UE4/Assets/Objects/Properties/StrProperty.cs
// (The AoC FName-reader special case is omitted with that game's reader.)
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class StrProperty : public TPropertyTagType<std::string>
    {
    public:
        StrProperty() { Value = std::string(); }
        explicit StrProperty(std::string value) { Value = std::move(value); }
        StrProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? std::string() : Ar.ReadFString(); }
        const char* TypeName() const override { return "StrProperty"; }
    };
}
