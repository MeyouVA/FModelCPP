// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAudioEventData.cs
// Asset data for one event. C# reads MediaList as ResolvedObject[]; the port keeps the raw package indices,
// since resolving them needs the loader (see the ObjectProperty note in PropertyUtil.h).
#pragma once

#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/UScriptArray.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/ObjectResource.h"
#include "UAkAssetDataSwitchContainer.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::UScriptArray;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UAkAudioEventData : public UAkAssetDataSwitchContainer
    {
    public:
        std::vector<FPackageIndex> MediaList;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAssetDataSwitchContainer::Deserialize(Ar, validPos);

            const UScriptArray* media = nullptr;
            if (!PropertyUtil::TryGet<const UScriptArray*>(*this, "MediaList", media) || media == nullptr) return;
            for (const auto& element : media->Properties)
            {
                const FPackageIndex* index = nullptr;
                if (element != nullptr && PropertyUtil::PropertyValue(*element, index) && index != nullptr)
                    MediaList.push_back(*index);
            }
        }
    };
}
