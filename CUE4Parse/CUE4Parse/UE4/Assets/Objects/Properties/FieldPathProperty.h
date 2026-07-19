// Ported from CUE4Parse/UE4/Assets/Objects/Properties/FieldPathProperty.cs
// A field-path reference value (FFieldPath = a chain of FNames + resolved owner).
//
// Deliberate difference: derives FPropertyTagType directly (aggregate-style); ToString is overridden.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/UObject/FFieldPath.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FFieldPath;

    class FieldPathProperty : public FPropertyTagType
    {
    public:
        FFieldPath Value;

        FieldPathProperty(FAssetArchive& Ar, ReadType type)
            : Value(type == ReadType::ZERO ? FFieldPath() : FFieldPath(Ar)) {}
        explicit FieldPathProperty(FFieldPath value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "FieldPathProperty"; }
        std::string ToString() const override { return Value.ToString() + " (FieldPathProperty)"; }
    };
}
