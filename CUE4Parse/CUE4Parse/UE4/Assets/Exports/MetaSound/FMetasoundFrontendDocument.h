// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendDocument.cs
// [StructFallback]. The whole MetaSound: its root graph, subgraphs and the classes they depend on.
#pragma once

#include <vector>

#include "FMetasoundFrontendDocumentMetadata.h"
#include "FMetasoundFrontendGraphClass.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendDocument
    {
    public:
        FMetasoundFrontendDocumentMetadata Metadata;
        std::vector<FMetasoundFrontendVersion> Interfaces;
        FMetasoundFrontendGraphClass RootGraph;
        std::vector<FMetasoundFrontendGraphClass> Subgraphs;
        std::vector<FMetasoundFrontendClass> Dependencies;

        FMetasoundFrontendDocument() = default;

        explicit FMetasoundFrontendDocument(const FStructFallback& fallback)
        {
            Metadata = PropertyUtil::GetOrDefault<FMetasoundFrontendDocumentMetadata>(fallback, "Metadata");
            Interfaces = PropertyUtil::GetStructArray<FMetasoundFrontendVersion>(fallback, "Interfaces");
            RootGraph = PropertyUtil::GetOrDefault<FMetasoundFrontendGraphClass>(fallback, "RootGraph");
            Subgraphs = PropertyUtil::GetStructArray<FMetasoundFrontendGraphClass>(fallback, "Subgraphs");
            Dependencies = PropertyUtil::GetStructArray<FMetasoundFrontendClass>(fallback, "Dependencies");
        }
    };
}
