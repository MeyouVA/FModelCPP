// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/USoundNode.cs
// A node in a sound cue's graph: its children come from a tagged property, then a strip-flags pair.
#pragma once

#include <vector>

#include "../../UObject.h"
#include "../../PropertyUtil.h"
#include "../../../Readers/FAssetArchive.h"
#include "../../../../Objects/Engine/FStripDataFlags.h"
#include "../../../../Objects/UObject/ObjectResource.h"
#include "../../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    using CUE4Parse::UE4::Objects::Engine::FStripDataFlags;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;

    class USoundNode : public UObject
    {
    public:
        std::vector<FPackageIndex> ChildNodes;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            ChildNodes = PropertyUtil::GetArray<FPackageIndex>(*this, "ChildNodes");
            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::COOKED_ASSETS_IN_EDITOR_SUPPORT)
                (void) FStripDataFlags(Ar);
        }
    };
}
