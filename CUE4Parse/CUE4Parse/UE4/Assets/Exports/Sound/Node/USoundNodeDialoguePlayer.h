// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/USoundNodeDialoguePlayer.cs
#pragma once

#include "FDialogueWaveParameter.h"
#include "USoundNode.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    class USoundNodeDialoguePlayer : public USoundNode
    {
    public:
        FDialogueWaveParameter DialogueWaveParameter;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            USoundNode::Deserialize(Ar, validPos);

            DialogueWaveParameter = PropertyUtil::GetOrDefault<FDialogueWaveParameter>(*this, "DialogueWaveParameter");
        }
    };
}
