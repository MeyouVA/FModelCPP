// Ported from CUE4Parse/UE4/Assets/Exports/Sound/UDialogueWave.cs
// Maps speaker/target contexts onto the sound waves that voice them.
#pragma once

#include <string>
#include <vector>

#include "Node/FDialogueContextMapping.h"
#include "../UObject.h"
#include "../PropertyUtil.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using Node::FDialogueContext;
    using Node::FDialogueContextMapping;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UDialogueWave : public UObject
    {
    public:
        std::string SpokenText;
        std::vector<FDialogueContextMapping> ContextMappings;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            SpokenText = PropertyUtil::GetOrDefault<std::string>(*this, "SpokenText");
            ContextMappings = PropertyUtil::GetStructArray<FDialogueContextMapping>(*this, "ContextMappings");

            (void) Ar.ReadBoolean();
        }

        FPackageIndex GetWaveFromContext(const FDialogueContext& context) const
        {
            for (const auto& contextMapping : ContextMappings)
            {
                if (contextMapping.Context == context)
                    return contextMapping.SoundWave;
            }

            return FPackageIndex();
        }
    };
}
