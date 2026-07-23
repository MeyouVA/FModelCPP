// Ported from CUE4Parse/UE4/Assets/Objects/FStructFallback.cs
// A generic struct value: just a bag of tagged properties (the fallback when a struct has no dedicated
// C++ reader). Doubles as the property holder for tagged struct data. Implements IUStruct.
//
// Deliberate differences from C#:
//   * Tagged AND unversioned paths are ported (the unversioned one builds a stack UScriptClass over
//     structType, as C# news one up). The raw-header ctor (DeserializeRawProperties) is omitted. TODO.
//   * The reflection-based property accessors on AbstractPropertyHolder are omitted; consumers read the
//     Properties vector directly (mirrors the note on UObject).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "FPropertyTag.h"
#include "../../IUStruct.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }
namespace CUE4Parse::UE4::Objects::UObject { class UStruct; }

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::IUStruct;

    class FStructFallback : public IUStruct
    {
    public:
        std::vector<FPropertyTag> Properties;

        FStructFallback() = default;
        // Reads struct properties: unversioned (via structType's mappings schema) when the package has
        // unversioned properties, tagged otherwise.
        explicit FStructFallback(Readers::FAssetArchive& Ar, const std::optional<std::string>& structType = std::nullopt);
        // C#'s FStructFallback(Ar, UStruct?) overload (unversioned via a loaded struct's schema).
        FStructFallback(Readers::FAssetArchive& Ar, const CUE4Parse::UE4::Objects::UObject::UStruct* structType);

        // Move-only (FPropertyTag is move-only).
        FStructFallback(FStructFallback&&) = default;
        FStructFallback& operator=(FStructFallback&&) = default;
        FStructFallback(const FStructFallback&) = delete;
        FStructFallback& operator=(const FStructFallback&) = delete;

        std::string ToString() const;
    };
}
