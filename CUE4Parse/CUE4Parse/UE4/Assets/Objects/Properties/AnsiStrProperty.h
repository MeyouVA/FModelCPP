// Ported from CUE4Parse/UE4/Assets/Objects/Properties/AnsiStrProperty.cs
// A StrProperty whose value is serialized as an ANSI FString.
#pragma once

#include <string>

#include "StrProperty.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class AnsiStrProperty : public StrProperty
    {
    public:
        AnsiStrProperty() { Value = std::string(); }
        explicit AnsiStrProperty(std::string value) { Value = std::move(value); }
        AnsiStrProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? std::string() : Ar.ReadFAnsiString(); }
        const char* TypeName() const override { return "AnsiStrProperty"; }
    };
}
