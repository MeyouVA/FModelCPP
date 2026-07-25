// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassMetadata.cs
// [StructFallback].
//
// Faithful quirk: C# reads Type with `nameof(EMetasoundFrontendClassType)` -- the *enum's* name, not the
// field's ("Type"). No property is called that, so Type always comes back as the default. Kept as-is.
#pragma once

#include <cstdint>

#include "FMetasoundFrontendClassName.h"
#include "FMetasoundFrontendVersionNumber.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    enum class EMetasoundFrontendClassType : uint8_t
    {
        // The MetaSound class is defined externally, in compiled code or in another document.
        External = 0,

        // The MetaSound class is a graph within the containing document.
        Graph,

        // The MetaSound class is an input into a graph in the containing document.
        Input,

        // The MetaSound class is an output from a graph in the containing document.
        Output,

        // The MetaSound class is an literal requiring a literal value to construct.
        Literal,

        // The MetaSound class is an variable requiring a literal value to construct.
        Variable,

        // The MetaSound class accesses variables.
        VariableDeferredAccessor,

        // The MetaSound class accesses variables.
        VariableAccessor,

        // The MetaSound class mutates variables.
        VariableMutator,

        // The MetaSound class is defined only by the Frontend, and associatively
        // performs a functional operation within the given document in a registration/cook step.
        Template,

        Invalid,
    };

    // [Flags]
    enum class EMetasoundFrontendClassAccessFlags : uint16_t
    {
        None = 0,

        // Class is marked as deprecated when referenced by
        // MetaSounds in the editor.
        Deprecated = 1 << 0,

        // If set, MetaSound can be referenced by other MetaSounds in either
        // editor or by builder Blueprint API.
        Referenceable = 1 << 1,

        Default = Referenceable,
    };

    class FMetasoundFrontendClassMetadata
    {
    public:
        FMetasoundFrontendClassName ClassName;
        FMetasoundFrontendVersionNumber Version;
        EMetasoundFrontendClassType Type = EMetasoundFrontendClassType::External;
        EMetasoundFrontendClassAccessFlags AccessFlags = EMetasoundFrontendClassAccessFlags::None;

        FMetasoundFrontendClassMetadata() = default;

        explicit FMetasoundFrontendClassMetadata(const FStructFallback& fallback)
        {
            ClassName = PropertyUtil::GetOrDefault<FMetasoundFrontendClassName>(fallback, "ClassName");
            Version = PropertyUtil::GetOrDefault<FMetasoundFrontendVersionNumber>(fallback, "Version");
            Type = PropertyUtil::GetOrDefault<EMetasoundFrontendClassType>(fallback, "EMetasoundFrontendClassType");
            AccessFlags = static_cast<EMetasoundFrontendClassAccessFlags>(
                PropertyUtil::GetOrDefault<uint16_t>(fallback, "AccessFlags"));
        }
    };
}
