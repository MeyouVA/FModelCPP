// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/UMetaSoundPatch.cs
// A MetaSound graph asset with no audio output of its own.
#pragma once

#include <string>
#include <vector>

#include "FMetasoundFrontendDocument.h"
#include "../UObject.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UMetaSoundPatch : public UObject
    {
    public:
        FMetasoundFrontendDocument RootMetaSoundDocument;
        std::vector<std::string> ReferencedAssetClassKeys;
        std::vector<FPackageIndex> ReferencedAssetClassObjects;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            RootMetaSoundDocument = PropertyUtil::GetOrDefault<FMetasoundFrontendDocument>(*this, "RootMetaSoundDocument");
            ReferencedAssetClassKeys = PropertyUtil::GetArray<std::string>(*this, "ReferencedAssetClassKeys");
            ReferencedAssetClassObjects = PropertyUtil::GetArray<FPackageIndex>(*this, "ReferencedAssetClassObjects");
        }
    };
}
