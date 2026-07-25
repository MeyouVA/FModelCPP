// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/BaseEffectNode.cs
// Abstract base for the effect node family; EffectBody is attached after construction. Stored in
// FModReader::EffectNodes as unique_ptr<BaseEffectNode>, so it carries a virtual destructor.
#pragma once

#include <memory>

#include "EffectNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class BaseEffectNode
    {
    public:
        std::unique_ptr<EffectNode> EffectBody;

        virtual ~BaseEffectNode() = default;
    };
}
