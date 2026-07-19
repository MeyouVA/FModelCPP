// Ported from CUE4Parse/UE4/Assets/Objects/Properties/SetProperty.cs
// A SetProperty value: wraps a UScriptSet. (Derives FPropertyTagType directly — see the note on StructProperty.)
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../UScriptSet.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class SetProperty : public FPropertyTagType
    {
    public:
        UScriptSet Value;

        SetProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type);
        explicit SetProperty(UScriptSet value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "SetProperty"; }
        std::string ToString() const override { return Value.ToString() + " (SetProperty)"; }
    };
}
