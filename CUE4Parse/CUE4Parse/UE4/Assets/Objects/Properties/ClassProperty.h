// Ported from CUE4Parse/UE4/Assets/Objects/Properties/ClassProperty.cs
// A class reference, serialized identically to ObjectProperty (an FPackageIndex).
#pragma once

#include "ObjectProperty.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class ClassProperty : public ObjectProperty
    {
    public:
        ClassProperty(FAssetArchive& Ar, ReadType type) : ObjectProperty(Ar, type) {}
        explicit ClassProperty(FPackageIndex value) : ObjectProperty(std::move(value)) {}

        const char* TypeName() const override { return "ClassProperty"; }
    };
}
