// Ported from CUE4Parse/UE4/IO/Objects/FPackageObjectIndex.cs (+ FPackageImportReference.cs)
// A Zen object reference: 2 type bits + 62 index/hash bits. Exports carry a local export index, script
// imports a global script-object hash, package imports a (package index, public-export-hash index) pair.
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::IO::Objects
{
    struct FPackageImportReference
    {
        uint32_t ImportedPackageIndex = 0;
        uint32_t ImportedPublicExportHashIndex = 0;
    };

    // C#'s namespace-level EType (FPackageObjectIndex's type field).
    enum class EType : uint32_t
    {
        Export,
        ScriptImport,
        PackageImport,
        Null,
        TypeCount = Null
    };

    struct FPackageObjectIndex
    {
        static constexpr int Size = sizeof(uint64_t);
        static constexpr int IndexBits = 62;
        static constexpr uint64_t IndexMask = (1ull << IndexBits) - 1ull;
        static constexpr uint64_t TypeMask = ~IndexMask;
        static constexpr int TypeShift = IndexBits;
        static constexpr uint64_t Invalid = ~0ull;

        uint64_t TypeAndId = Invalid;

        FPackageObjectIndex() = default;
        explicit FPackageObjectIndex(uint64_t typeAndId) : TypeAndId(typeAndId) {}

        static FPackageObjectIndex InvalidObjectIndex() { return FPackageObjectIndex(Invalid); }

        EType Type() const { return static_cast<EType>(TypeAndId >> TypeShift); }
        uint64_t Value() const { return TypeAndId & IndexMask; }

        bool IsNull() const { return TypeAndId == Invalid; }
        bool IsExport() const { return Type() == EType::Export; }
        bool IsImport() const { return IsScriptImport() || IsPackageImport(); }
        bool IsScriptImport() const { return Type() == EType::ScriptImport; }
        bool IsPackageImport() const { return Type() == EType::PackageImport; }
        uint32_t AsExport() const { return static_cast<uint32_t>(TypeAndId); }

        FPackageImportReference AsPackageImportRef() const
        {
            return FPackageImportReference{
                static_cast<uint32_t>((TypeAndId & IndexMask) >> 32),
                static_cast<uint32_t>(TypeAndId)
            };
        }

        bool operator==(const FPackageObjectIndex& other) const { return TypeAndId == other.TypeAndId; }
        bool operator!=(const FPackageObjectIndex& other) const { return TypeAndId != other.TypeAndId; }
        // Map-key ordering (C# uses Dictionary + GetHashCode; std::map needs an order).
        bool operator<(const FPackageObjectIndex& other) const { return TypeAndId < other.TypeAndId; }
    };
    static_assert(sizeof(FPackageObjectIndex) == 8);
}
