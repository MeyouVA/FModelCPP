// Ported from CUE4Parse/UE4/Assets/Objects/Properties/LazyObjectProperty.cs
// A lazy object reference stored as an FUniqueObjectGuid (a single FGuid).
//
// Deliberate difference: derives FPropertyTagType directly (aggregate-style, see ObjectProperty) rather than
// TPropertyTagType<FUniqueObjectGuid>, so no scalar PropValueToString overload is needed; ToString is overridden.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/UObject/FUniqueObjectGuid.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FUniqueObjectGuid;

    class LazyObjectProperty : public FPropertyTagType
    {
    public:
        FUniqueObjectGuid Value;

        LazyObjectProperty(FAssetArchive& Ar, ReadType type);
        explicit LazyObjectProperty(FUniqueObjectGuid value) : Value(value) {}

        const char* TypeName() const override { return "LazyObjectProperty"; }
        std::string ToString() const override { return Value.ToString() + " (LazyObjectProperty)"; }
    };
}
