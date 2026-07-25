// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendDocumentMetadata.cs
// [StructFallback].
#pragma once

#include "FMetasoundFrontendVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendDocumentMetadata
    {
    public:
        FMetasoundFrontendVersion Version;

        FMetasoundFrontendDocumentMetadata() = default;

        explicit FMetasoundFrontendDocumentMetadata(const FStructFallback& fallback)
        {
            Version = PropertyUtil::GetOrDefault<FMetasoundFrontendVersion>(fallback, "Version");
        }
    };
}
