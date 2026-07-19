// Ported from CUE4Parse/UE4/Assets/Objects/Properties/DelegateProperty.cs
// A single-cast script delegate value (FScriptDelegate).
//
// Deliberate difference: derives FPropertyTagType directly (aggregate-style, see ObjectProperty) rather than
// TPropertyTagType<FScriptDelegate>; ToString is overridden.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/UObject/ScriptDelegates.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FScriptDelegate;

    class DelegateProperty : public FPropertyTagType
    {
    public:
        FScriptDelegate Value;

        DelegateProperty(FAssetArchive& Ar, ReadType type);
        explicit DelegateProperty(FScriptDelegate value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "DelegateProperty"; }
        std::string ToString() const override { return Value.ToString() + " (DelegateProperty)"; }
    };
}
