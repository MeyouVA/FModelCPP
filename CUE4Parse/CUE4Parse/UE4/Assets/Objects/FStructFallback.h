// Ported from CUE4Parse/UE4/Assets/Objects/FStructFallback.cs
// A generic struct value: just a bag of tagged properties (the fallback when a struct has no dedicated
// C++ reader). Doubles as the property holder for tagged struct data. Implements IUStruct.
//
// Deliberate differences from C#:
//   * Only the *tagged* (versioned) path is ported. The unversioned ctor (DeserializePropertiesUnversioned,
//     needs a mappings provider) and the raw-header ctor (DeserializeRawProperties) throw / are omitted. TODO.
//   * The reflection-based property accessors on AbstractPropertyHolder are omitted; consumers read the
//     Properties vector directly (mirrors the note on UObject).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "FPropertyTag.h"
#include "../../IUStruct.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::IUStruct;

    class FStructFallback : public IUStruct
    {
    public:
        std::vector<FPropertyTag> Properties;

        FStructFallback() = default;
        // Reads tagged struct properties. structType is only consulted on the (deferred) unversioned path.
        explicit FStructFallback(Readers::FAssetArchive& Ar, const std::optional<std::string>& structType = std::nullopt);

        // Move-only (FPropertyTag is move-only).
        FStructFallback(FStructFallback&&) = default;
        FStructFallback& operator=(FStructFallback&&) = default;
        FStructFallback(const FStructFallback&) = delete;
        FStructFallback& operator=(const FStructFallback&) = delete;

        std::string ToString() const;
    };
}
