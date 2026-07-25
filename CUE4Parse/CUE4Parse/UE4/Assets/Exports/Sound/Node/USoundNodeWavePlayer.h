// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/USoundNodeWavePlayer.cs
// Deliberate difference from C#: WriteJson is dropped (the port has no JSON serializer layer).
#pragma once

#include <optional>

#include "USoundNodeAssetReferencer.h"
#include "../../../../Objects/UObject/FSoftObjectPath.h"
#include "../../../../Versions/FFrameworkObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    using CUE4Parse::UE4::Objects::UObject::FSoftObjectPath;
    // FFrameworkObjectVersion is a namespace (C# static class), so it is brought in via the namespace itself.
    using namespace CUE4Parse::UE4::Versions;

    class USoundNodeWavePlayer : public USoundNodeAssetReferencer
    {
    public:
        std::optional<FPackageIndex> SoundWave;
        FSoftObjectPath SoundWaveAssetPtr;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            USoundNodeAssetReferencer::Deserialize(Ar, validPos);

            if (FFrameworkObjectVersion::Get(Ar) >= FFrameworkObjectVersion::HardSoundReferences)
                SoundWave.emplace(Ar);

            SoundWaveAssetPtr = PropertyUtil::GetOrDefault<FSoftObjectPath>(*this, "SoundWaveAssetPtr");
        }
    };
}
