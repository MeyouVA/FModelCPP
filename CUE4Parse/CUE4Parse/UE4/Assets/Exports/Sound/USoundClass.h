// Ported from CUE4Parse/UE4/Assets/Exports/Sound/USoundClass.cs
// Only the UE3-era editor graph positions are serialized; UE4 moved them into the editor-only graph object.
#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "../UObject.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/ObjectResource.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE3Version;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;

    struct FSoundClassEditorData
    {
        int32_t X = 0;
        int32_t Y = 0;
    };

    class USoundClass : public UObject
    {
    public:
        std::optional<std::vector<std::pair<FPackageIndex, FSoundClassEditorData>>> EditorData;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);
            if (Ar.Ver() >= EUnrealEngineObjectUE3Version::SOUND_CLASS_SERIALISATION_UPDATE &&
                Ar.Ver() < EUnrealEngineObjectUE4Version::SOUND_CLASS_GRAPH_EDITOR)
            {
                const int32_t count = Ar.Read<int32_t>();
                std::vector<std::pair<FPackageIndex, FSoundClassEditorData>> pairs;
                pairs.reserve(static_cast<size_t>(count < 0 ? 0 : count));
                for (int32_t i = 0; i < count; i++)
                {
                    // Key before value: one C# expression, two statements here.
                    FPackageIndex key(Ar);
                    pairs.emplace_back(key, Ar.Read<FSoundClassEditorData>());
                }
                EditorData = std::move(pairs);
            }
        }
    };
}
