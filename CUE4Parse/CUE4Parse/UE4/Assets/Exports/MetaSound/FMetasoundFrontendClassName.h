// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassName.cs
// [StructFallback]. The three-part name a MetaSound class is registered under.
#pragma once

#include "../PropertyUtil.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FMetasoundFrontendClassName
    {
    public:
        FName Namespace;
        FName Name;
        FName Variant;

        FMetasoundFrontendClassName() = default;

        explicit FMetasoundFrontendClassName(const FStructFallback& fallback)
        {
            Namespace = PropertyUtil::GetOrDefault<FName>(fallback, "Namespace");
            Name = PropertyUtil::GetOrDefault<FName>(fallback, "Name");
            Variant = PropertyUtil::GetOrDefault<FName>(fallback, "Variant");
        }
    };
}
