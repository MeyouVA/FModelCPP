// Ported from CUE4Parse/UE4/Assets/Objects/Properties/MulticastDelegateProperty.cs
// A multicast script delegate value (FMulticastScriptDelegate), plus the Inline/Sparse subclasses that
// serialize identically.
//
// Deliberate difference: derives FPropertyTagType directly (aggregate-style); ToString uses the virtual
// TypeName() so the Inline/Sparse subclasses print their own name (mirroring C#'s GetType().Name).
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/UObject/ScriptDelegates.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FMulticastScriptDelegate;

    class MulticastDelegateProperty : public FPropertyTagType
    {
    public:
        FMulticastScriptDelegate Value;

        MulticastDelegateProperty(FAssetArchive& Ar, ReadType type);
        explicit MulticastDelegateProperty(FMulticastScriptDelegate value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "MulticastDelegateProperty"; }
        std::string ToString() const override { return Value.ToString() + " (" + TypeName() + ")"; }
    };

    class MulticastInlineDelegateProperty : public MulticastDelegateProperty
    {
    public:
        MulticastInlineDelegateProperty(FAssetArchive& Ar, ReadType type) : MulticastDelegateProperty(Ar, type) {}
        explicit MulticastInlineDelegateProperty(FMulticastScriptDelegate value) : MulticastDelegateProperty(std::move(value)) {}
        const char* TypeName() const override { return "MulticastInlineDelegateProperty"; }
    };

    class MulticastSparseDelegateProperty : public MulticastDelegateProperty
    {
    public:
        MulticastSparseDelegateProperty(FAssetArchive& Ar, ReadType type) : MulticastDelegateProperty(Ar, type) {}
        explicit MulticastSparseDelegateProperty(FMulticastScriptDelegate value) : MulticastDelegateProperty(std::move(value)) {}
        const char* TypeName() const override { return "MulticastSparseDelegateProperty"; }
    };
}
