// Ported from CUE4Parse/UE4/Assets/Exports/UObjectRedirector.cs
// A UObjectRedirector export: a stub object left behind when an asset is renamed/moved, pointing at the
// object that replaced it.
//
// Deliberate difference from C#: DestinationObject is a plain FPackageIndex value rather than a nullable
// one. C# declares it FPackageIndex? but always assigns `new FPackageIndex(Ar)` (never null); an unset
// value is representable as the null index (Index == 0). WriteJson is omitted.
#pragma once

#include <cstdint>

#include "UObject.h"
#include "../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Exports
{
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UObjectRedirector : public UObject
    {
    public:
        FPackageIndex DestinationObject; // The object this redirector forwards to.

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
