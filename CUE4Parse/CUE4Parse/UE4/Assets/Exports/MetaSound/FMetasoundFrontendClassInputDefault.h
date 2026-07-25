// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassInputDefault.cs
// [StructFallback]. One page's default literal for a class input.
#pragma once

#include "FMetasoundFrontendLiteral.h"
#include "../../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

    class FMetasoundFrontendClassInputDefault
    {
    public:
        FMetasoundFrontendLiteral Literal;
        FGuid PageID;

        FMetasoundFrontendClassInputDefault() = default;

        explicit FMetasoundFrontendClassInputDefault(const FStructFallback& fallback)
        {
            Literal = PropertyUtil::GetOrDefault<FMetasoundFrontendLiteral>(fallback, "Literal");
            PageID = PropertyUtil::GetOrDefault<FGuid>(fallback, "PageID");
        }
    };
}
