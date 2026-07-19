// Ported from CUE4Parse/UE4/Assets/Objects/FScriptStruct.cs
// The value of a StructProperty: a resolved struct instance. In C# StructType is an IUStruct chosen from a
// ~200-entry switch of named engine/game structs (FVector, FGuid, FColor, ...), falling back to FStructFallback.
//
// Deliberate differences from C#:
//   * The entire named-struct switch is deferred until the Core/Math + engine struct types are ported; this
//     port always takes the generic FStructFallback path (the C# `default:` arm). So StructType is stored as a
//     concrete FStructFallback rather than an IUStruct base. TODO: add the named-struct table.
//   * The `struc` (UStruct*) parameter and the ReadInstancedStruct* helpers (which need object loading) are
//     omitted. TODO.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "FStructFallback.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    namespace Properties { enum class ReadType : uint8_t; }

    class FScriptStruct
    {
    public:
        // Only the fallback struct type is supported so far (see header note).
        std::unique_ptr<FStructFallback> Struct;

        FScriptStruct() = default;
        FScriptStruct(Readers::FAssetArchive& Ar, const std::optional<std::string>& structName, Properties::ReadType type);
        explicit FScriptStruct(std::unique_ptr<FStructFallback> value) : Struct(std::move(value)) {}

        // Move-only (owns a unique_ptr to a move-only holder).
        FScriptStruct(FScriptStruct&&) = default;
        FScriptStruct& operator=(FScriptStruct&&) = default;
        FScriptStruct(const FScriptStruct&) = delete;
        FScriptStruct& operator=(const FScriptStruct&) = delete;

        std::string ToString() const;
    };
}
