// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassEnvironmentVariable.cs
// [StructFallback]. A value a class expects from its execution environment rather than from a pin.
#pragma once

#include "../PropertyUtil.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FMetasoundFrontendClassEnvironmentVariable
    {
    public:
        FName Name;
        FName TypeName;
        bool bIsRequired = false;

        FMetasoundFrontendClassEnvironmentVariable() = default;

        explicit FMetasoundFrontendClassEnvironmentVariable(const FStructFallback& fallback)
        {
            Name = PropertyUtil::GetOrDefault<FName>(fallback, "Name");
            TypeName = PropertyUtil::GetOrDefault<FName>(fallback, "TypeName");
            bIsRequired = PropertyUtil::GetOrDefault<bool>(fallback, "bIsRequired");
        }
    };
}
