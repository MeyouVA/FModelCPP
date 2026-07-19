// Ported from CUE4Parse/UE4/Assets/Objects/Properties/VerseStringProperty.cs
// A StrProperty whose value is serialized as a UTF-8 FString (Verse string).
#pragma once

#include <string>

#include "StrProperty.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class VerseStringProperty : public StrProperty
    {
    public:
        VerseStringProperty() { Value = std::string(); }
        explicit VerseStringProperty(std::string value) { Value = std::move(value); }
        VerseStringProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? std::string() : Ar.ReadFUtf8String(); }
        const char* TypeName() const override { return "VerseStringProperty"; }
    };
}
