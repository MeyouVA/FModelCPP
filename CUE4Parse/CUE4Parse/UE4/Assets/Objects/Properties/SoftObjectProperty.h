// Ported from CUE4Parse/UE4/Assets/Objects/Properties/SoftObjectProperty.cs
// A soft (by-name) object reference stored as an FSoftObjectPath. Also the reader for SoftClassProperty and
// ReferenceProperty, which serialize identically.
//
// Deliberate difference: derives FPropertyTagType directly (aggregate-style, see ObjectProperty) rather than
// TPropertyTagType<FSoftObjectPath>, so no scalar PropValueToString overload is needed; ToString is overridden.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/UObject/FSoftObjectPath.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FSoftObjectPath;

    class SoftObjectProperty : public FPropertyTagType
    {
    public:
        FSoftObjectPath Value;

        SoftObjectProperty(FAssetArchive& Ar, ReadType type);
        explicit SoftObjectProperty(FSoftObjectPath value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "SoftObjectProperty"; }
        std::string ToString() const override { return Value.ToString() + " (SoftObjectProperty)"; }
    };
}
