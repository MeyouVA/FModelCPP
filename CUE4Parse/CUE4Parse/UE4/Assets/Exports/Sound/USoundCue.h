// Ported from CUE4Parse/UE4/Assets/Exports/Sound/USoundCue.cs
// The root of a sound-node graph: everything of interest is a tagged property, plus a trailing strip-flags pair.
#pragma once

#include "USoundBase.h"
#include "../PropertyUtil.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Engine/FStripDataFlags.h"
#include "../../../Objects/UObject/ObjectResource.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Objects::Engine::FStripDataFlags;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;

    class USoundCue : public USoundBase
    {
    public:
        FPackageIndex FirstNode;
        float VolumeMultiplier = 0.0f;
        float PitchMultiplier = 0.0f;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            FirstNode = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "FirstNode");
            VolumeMultiplier = PropertyUtil::GetOrDefault<float>(*this, "VolumeMultiplier", 0.75f);
            PitchMultiplier = PropertyUtil::GetOrDefault<float>(*this, "PitchMultiplier", 1.0f);

            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::COOKED_ASSETS_IN_EDITOR_SUPPORT)
                (void) FStripDataFlags(Ar);
        }
    };
}
