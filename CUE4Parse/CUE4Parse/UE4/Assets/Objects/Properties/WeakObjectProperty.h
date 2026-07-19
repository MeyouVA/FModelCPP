// Ported from CUE4Parse/UE4/Assets/Objects/Properties/WeakObjectProperty.cs
// A weak object reference, serialized identically to ObjectProperty (an FPackageIndex).
#pragma once

#include "ObjectProperty.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class WeakObjectProperty : public ObjectProperty
    {
    public:
        WeakObjectProperty(FAssetArchive& Ar, ReadType type) : ObjectProperty(Ar, type) {}
        explicit WeakObjectProperty(FPackageIndex value) : ObjectProperty(std::move(value)) {}

        const char* TypeName() const override { return "WeakObjectProperty"; }
    };
}
