// Ported from CUE4Parse/UE4/FMod/Nodes/Transitions/BaseTransitionNode.cs
// Abstract base for the transition node family; TransitionBody is attached after construction. Stored in
// FModReader::TransitionNodes as unique_ptr<BaseTransitionNode>, so it carries a virtual destructor.
#pragma once

#include <memory>

#include "TransitionTimelineNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Transitions
{
    class BaseTransitionNode
    {
    public:
        std::unique_ptr<TransitionTimelineNode> TransitionBody;

        virtual ~BaseTransitionNode() = default;
    };
}
