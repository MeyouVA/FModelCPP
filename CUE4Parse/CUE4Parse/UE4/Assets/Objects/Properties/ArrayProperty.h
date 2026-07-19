// Ported from CUE4Parse/UE4/Assets/Objects/Properties/ArrayProperty.cs
// An ArrayProperty value: wraps a UScriptArray.
//
// Deliberate difference from C#: derives from FPropertyTagType directly rather than TPropertyTagType<UScriptArray>
// (see the note on StructProperty); ToString mirrors the generic base format "{Value} (ArrayProperty)".
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../UScriptArray.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class ArrayProperty : public FPropertyTagType
    {
    public:
        UScriptArray Value;

        ArrayProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type, int size = 0);
        explicit ArrayProperty(UScriptArray value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "ArrayProperty"; }
        std::string ToString() const override { return Value.ToString() + " (ArrayProperty)"; }
    };
}
