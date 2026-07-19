// Ported from CUE4Parse/UE4/Assets/Objects/Properties/Utf8StrProperty.cs
// A StrProperty whose value is serialized as a UTF-8 FString.
#pragma once

#include <string>

#include "StrProperty.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class Utf8StrProperty : public StrProperty
    {
    public:
        Utf8StrProperty() { Value = std::string(); }
        explicit Utf8StrProperty(std::string value) { Value = std::move(value); }
        Utf8StrProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? std::string() : Ar.ReadFUtf8String(); }
        const char* TypeName() const override { return "Utf8StrProperty"; }
    };
}
