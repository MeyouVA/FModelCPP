// Ported from CUE4Parse/UE4/Assets/Objects/Properties/TextProperty.cs
// An FText property value.
//
// Deliberate difference: derives FPropertyTagType directly (aggregate-style, see ObjectProperty) rather than
// TPropertyTagType<FText>; ToString is overridden.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/Core/i18N/FText.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::Core::i18N::FText;

    class TextProperty : public FPropertyTagType
    {
    public:
        FText Value;

        TextProperty(FAssetArchive& Ar, ReadType type);
        explicit TextProperty(FText value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "TextProperty"; }
        std::string ToString() const override { return Value.Text() + " (TextProperty)"; }
    };
}
