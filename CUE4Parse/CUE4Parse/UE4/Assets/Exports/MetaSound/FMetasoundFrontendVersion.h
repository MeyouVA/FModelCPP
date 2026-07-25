// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendVersion.cs
// [StructFallback].
#pragma once

#include "FMetasoundFrontendVersionNumber.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FMetasoundFrontendVersion
    {
    public:
        FName Name;
        FMetasoundFrontendVersionNumber Number;

        FMetasoundFrontendVersion() = default;

        explicit FMetasoundFrontendVersion(const FStructFallback& fallback)
        {
            Name = PropertyUtil::GetOrDefault<FName>(fallback, "Name");
            Number = PropertyUtil::GetOrDefault<FMetasoundFrontendVersionNumber>(fallback, "Number");
        }
    };
}
