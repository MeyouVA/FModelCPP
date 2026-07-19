// Ported from CUE4Parse/UE4/Assets/Objects/Properties/OptionalProperty.cs
// An optional wrapper around another property value: a presence bool, then (if present) the inner property.
//
// Deliberate differences: derives FPropertyTagType directly (aggregate-style) holding a unique_ptr to the inner
// value (null when absent); ToString mirrors C#'s empty-string-when-null behaviour. The inner tag's InnerTypeData
// is not ported (deferred with FPropertyTagData), so nullptr is passed for it.
#pragma once

#include <memory>
#include <string>

#include "FPropertyTagType.h"

namespace CUE4Parse::UE4::Assets::Objects { class FPropertyTagData; }

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class OptionalProperty : public FPropertyTagType
    {
    public:
        // The inner property value, or null when the optional is absent.
        std::unique_ptr<FPropertyTagType> Value;

        OptionalProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type);
        explicit OptionalProperty(std::unique_ptr<FPropertyTagType> value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "OptionalProperty"; }
        std::string ToString() const override
        {
            return Value ? Value->ToString() + " (OptionalProperty)" : std::string();
        }
    };
}
