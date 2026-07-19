// Ported from CUE4Parse/UE4/Objects/UObject/FNameEntrySerialized.cs
// One entry in a package's serialized name pool: just a (nullable) string, plus the loaders that read a
// batch of them. Two shapes exist — the classic per-package FString entries (the FArchive ctor), and the
// IO Store "name batch" blob (LoadNameBatch + FSerializedNameHeader).
//
// FArchive is only forward-declared here so FName.h can include this header without pulling in FArchive.h
// (FArchive.h includes FName.h — including it back would cycle). The FArchive-dependent members are
// defined in FNameEntrySerialized.cpp.
//
// Deliberate differences from C#:
//   * NAME_HASHES is not defined (there are no hash fields), so the hash bytes are skipped (the C# #else
//     path: Ar.Position += 4).
//   * The PUBG name remap (an embedded PUBGNameHashMap.json resource) is not vendored, so it is skipped
//     with a TODO — matches how other embedded-resource lookups are deferred in this port.
//   * Name.Trim() trims ASCII whitespace only (no .NET culture-aware Unicode trimming).
//   * The IO Store name-batch string decode always treats UTF-16 as little-endian (as ReadFString does),
//     rather than routing char reads through the archive's endianness.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    struct FNameEntrySerialized
    {
        // C# `string? Name` — nullopt models the C# null (ToString/PlainText fall back to "None").
        std::optional<std::string> Name;

        FNameEntrySerialized() = default;
        explicit FNameEntrySerialized(std::optional<std::string> name) : Name(std::move(name)) {}

        // Reads a classic per-package name entry (FString + optional flags/hashes).
        explicit FNameEntrySerialized(Readers::FArchive& Ar);

        std::string ToString() const { return Name.value_or("None"); }

        // IO Store "name batch" loaders.
        static std::vector<FNameEntrySerialized> LoadNameBatch(Readers::FArchive& nameAr, int nameCount);
        static std::vector<FNameEntrySerialized> LoadNameBatch(Readers::FArchive& Ar);

    private:
        static FNameEntrySerialized LoadNameHeader(Readers::FArchive& Ar);
    };

    // [StructLayout(Pack = 1, Size = 2)] — trivially copyable POD, read via Read<FSerializedNameHeader>().
    struct FSerializedNameHeader
    {
        static constexpr int Size = 2;

        uint8_t _data0 = 0;
        uint8_t _data1 = 0;

        bool IsUtf16() const { return (_data0 & 0x80u) != 0; }
        uint32_t Length() const { return ((_data0 & 0x7Fu) << 8) + _data1; }

        bool operator==(const FSerializedNameHeader& other) const { return _data0 == other._data0 && _data1 == other._data1; }
        bool operator!=(const FSerializedNameHeader& other) const { return !(*this == other); }
    };
}
