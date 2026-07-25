// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/UMetaSoundSource.cs
// A MetaSound that is playable as a sound source: a procedural wave plus the frontend document.
//
// Deliberate difference from C#: WriteJson is dropped (the port has no JSON serializer layer).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "FMetasoundFrontendDocument.h"
#include "../Sound/USoundWaveProcedural.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/ObjectResource.h"
#include "../../../Versions/EGame.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using namespace CUE4Parse::UE4::Versions;

    enum class EMetaSoundOutputAudioFormat : uint8_t
    {
        Mono,
        Stereo,
        Quad,
        FiveDotOne,
        SevenDotOne,

        COUNT,
    };

    class UMetaSoundSource : public Sound::USoundWaveProcedural
    {
    public:
        std::optional<FStructFallback> Settings;
        FMetasoundFrontendDocument RootMetasoundDocument;
        std::vector<std::string> ReferencedAssetClassKeys;
        std::vector<FPackageIndex> ReferencedAssetClassObjects;
        EMetaSoundOutputAudioFormat OutputFormat = EMetaSoundOutputAudioFormat::Mono;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            Sound::USoundWaveProcedural::Deserialize(Ar, validPos);
            if (Ar.Game() >= GAME_UE5_4)
                Settings.emplace(Ar, std::optional<std::string>("MetaSoundQualitySettings"));

            RootMetasoundDocument = PropertyUtil::GetOrDefault<FMetasoundFrontendDocument>(*this, "RootMetasoundDocument");
            ReferencedAssetClassKeys = PropertyUtil::GetArray<std::string>(*this, "ReferencedAssetClassKeys");
            ReferencedAssetClassObjects = PropertyUtil::GetArray<FPackageIndex>(*this, "ReferencedAssetClassObjects");
            OutputFormat = PropertyUtil::GetOrDefault<EMetaSoundOutputAudioFormat>(*this, "OutputFormat");
        }
    };
}
