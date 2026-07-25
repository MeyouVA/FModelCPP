// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendVersionNumber.cs
// [StructFallback].
#pragma once

#include <cstdint>

#include "../PropertyUtil.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;

    class FMetasoundFrontendVersionNumber
    {
    public:
        int32_t Major = 0;
        int32_t Minor = 0;

        FMetasoundFrontendVersionNumber() = default;

        explicit FMetasoundFrontendVersionNumber(const FStructFallback& fallback)
        {
            Major = PropertyUtil::GetOrDefault<int32_t>(fallback, "Major");
            Minor = PropertyUtil::GetOrDefault<int32_t>(fallback, "Minor");
        }
    };
}
