// Ported from CUE4Parse/UE4/Assets/Objects/Properties/InterfaceProperty.cs
// An interface reference value (FScriptInterface = an FPackageIndex).
//
// Deliberate difference: derives FPropertyTagType directly (aggregate-style); ToString is overridden.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/UObject/ScriptInterface.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FScriptInterface;

    class InterfaceProperty : public FPropertyTagType
    {
    public:
        FScriptInterface Value;

        InterfaceProperty(FAssetArchive& Ar, ReadType type)
            : Value(type == ReadType::ZERO ? FScriptInterface() : FScriptInterface(Ar)) {}
        explicit InterfaceProperty(FScriptInterface value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "InterfaceProperty"; }
        std::string ToString() const override { return Value.ToString() + " (InterfaceProperty)"; }
    };
}
