// Ported from CUE4Parse/UE4/Assets/Exports/Sound/UMetaSoundSource.cs
// The class in this file is named UMetaUSoundSource in C# -- a typo that is kept, because it is a distinct
// type from Exports/MetaSound/UMetaSoundSource.cs (which is the registered one). Both are ported as they are.
#pragma once

#include <optional>

#include "USoundWaveProcedural.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Versions/EGame.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using namespace CUE4Parse::UE4::Versions;

    class UMetaUSoundSource : public USoundWaveProcedural
    {
    public:
        std::optional<FStructFallback> Settings;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            USoundWaveProcedural::Deserialize(Ar, validPos);
            if (Ar.Game() >= GAME_UE5_4)
            {
                Settings.emplace(Ar, std::optional<std::string>("MetaSoundQualitySettings"));
            }
        }
    };
}
