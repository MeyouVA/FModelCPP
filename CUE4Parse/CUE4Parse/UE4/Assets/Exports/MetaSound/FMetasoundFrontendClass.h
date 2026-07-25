// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClass.cs
// [StructFallback]. A class declaration inside a document.
#pragma once

#include "FMetasoundFrontendClassInterface.h"
#include "FMetasoundFrontendClassMetadata.h"
#include "../../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

    class FMetasoundFrontendClass
    {
    public:
        FGuid ID;
        FMetasoundFrontendClassMetadata Metadata;
        FMetasoundFrontendClassInterface Interface;

        FMetasoundFrontendClass() = default;
        virtual ~FMetasoundFrontendClass() = default;

        explicit FMetasoundFrontendClass(const FStructFallback& fallback)
        {
            ID = PropertyUtil::GetOrDefault<FGuid>(fallback, "ID");
            Metadata = PropertyUtil::GetOrDefault<FMetasoundFrontendClassMetadata>(fallback, "Metadata");
            Interface = PropertyUtil::GetOrDefault<FMetasoundFrontendClassInterface>(fallback, "Interface");
        }
    };
}
