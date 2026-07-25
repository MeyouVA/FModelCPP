// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/SoundNodeTypes.cs
// The sound-node zoo. Most are pure markers -- the base class already reads the child list and strip flags;
// two carry one extra tagged array each.
#pragma once

#include <vector>

#include "USoundNode.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    class USoundNodeDoppler : public USoundNode {};
    class USoundNodeAttenuation : public USoundNode {};
    class USoundNodeQualityLevel : public USoundNode {};
    class USoundNodeEnveloper : public USoundNode {};
    class USoundNodeDelay : public USoundNode {};

    class USoundNodeMixer : public USoundNode
    {
    public:
        std::vector<float> InputVolume;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            USoundNode::Deserialize(Ar, validPos);
            InputVolume = PropertyUtil::GetArray<float>(*this, "InputVolume");
        }
    };

    class USoundNodeModulator : public USoundNode {};

    class USoundNodeRandom : public USoundNode
    {
    public:
        std::vector<float> Weights;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            USoundNode::Deserialize(Ar, validPos);
            Weights = PropertyUtil::GetArray<float>(*this, "Weights");
        }
    };

    class USoundNodeDistanceCrossFade : public USoundNode {};
    class USoundNodeSwitch : public USoundNode {};
    class USoundNodeModulatorContinuous : public USoundNode {};
    class USoundNodeSoundClass : public USoundNode {};
    class USoundNodeParamCrossFade : public USoundNode {};
    class USoundNodeLooping : public USoundNode {};
    class USoundNodeBranch : public USoundNode {};
}
