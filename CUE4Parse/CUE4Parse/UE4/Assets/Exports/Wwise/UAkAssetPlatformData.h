// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAssetPlatformData.cs
// A package index to the asset data cooked for the current platform.
#pragma once

#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/ObjectResource.h"
#include "../UObject.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UAkAssetPlatformData : public UObject
    {
    public:
        FPackageIndex CurrentAssetData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);
            CurrentAssetData = FPackageIndex(Ar);
        }
    };
}
