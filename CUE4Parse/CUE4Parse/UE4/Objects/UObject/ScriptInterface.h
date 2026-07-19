// Ported from CUE4Parse/UE4/Objects/UObject/ScriptInterface.cs
// FScriptInterface: a reference to an object implementing an interface (just an FPackageIndex).
#pragma once

#include <string>

#include "ObjectResource.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    class FScriptInterface
    {
    public:
        FPackageIndex Object;

        FScriptInterface() = default;
        explicit FScriptInterface(Assets::Readers::FAssetArchive& Ar) : Object(Ar) {}
        explicit FScriptInterface(FPackageIndex object) : Object(std::move(object)) {}

        std::string ToString() const { return Object.ToString(); }
    };
}
