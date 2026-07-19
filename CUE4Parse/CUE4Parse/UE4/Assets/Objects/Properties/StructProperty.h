// Ported from CUE4Parse/UE4/Assets/Objects/Properties/StructProperty.cs
// A StructProperty value: wraps an FScriptStruct.
//
// Deliberate difference from C#: derives from FPropertyTagType directly rather than TPropertyTagType<FScriptStruct>,
// because there is no scalar PropValueToString for an aggregate payload and ToString is overridden here anyway.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../FScriptStruct.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class StructProperty : public FPropertyTagType
    {
    public:
        FScriptStruct Value;

        StructProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type);
        explicit StructProperty(FScriptStruct value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "StructProperty"; }
        std::string ToString() const override;
    };
}
