// Ported from CUE4Parse/UE4/Assets/Objects/Properties/ObjectProperty.cs
// An ObjectProperty value: an FPackageIndex referencing an object in this or another package.
//
// Deliberate differences from C#: derives FPropertyTagType directly (aggregate-style, see StructProperty), and
// the game-specific ObjectProperty reader special cases (FLevelSaveRecordArchive string form, FOW2ObjectsArchive)
// are omitted with those readers. TODO.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class ObjectProperty : public FPropertyTagType
    {
    public:
        FPackageIndex Value;

        ObjectProperty(FAssetArchive& Ar, ReadType type);
        explicit ObjectProperty(FPackageIndex value) : Value(std::move(value)) {}

        // TypeName() is virtual so subclasses (Weak/Class) print their own name, matching C#'s GetType().Name.
        const char* TypeName() const override { return "ObjectProperty"; }
        std::string ToString() const override { return Value.ToString() + " (" + TypeName() + ")"; }
    };
}
