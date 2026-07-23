// Ported from CUE4Parse/MappingsProvider/Usmap/UsmapParser.cs (+ UsmapArchiveExtensions.cs and
// UsmapProperties.cs, whose static helpers live here as free functions).
// Parses a .usmap file: magic + version (+ optional package versioning), a possibly-compressed body
// holding the name LUT, the enum tables and the struct schemas.
//
// Deliberate differences from C#:
//   * The parsed TypeMappings is owned by the parser via shared_ptr (C# leans on GC; Structs point back at
//     their TypeMappings, so its address must be stable).
//   * The Stream/byte[]/path convenience ctors become two: a path ctor and an FArchive ctor (callers wrap
//     bytes in an FByteArchive themselves, as the providers do).
//   * Decompression goes through this port's Compression::Decompress, so Oodle/Zstd need their decompressor
//     registered first (Brotli has no built-in decompressor yet and throws UnknownCompressionMethodException).
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "UsmapEnums.h"
#include "FUsmapReader.h"
#include "../MappingsSchema.h"
#include "../TypeMappings.h"
#include "../../UE4/Objects/Core/Serialization/FCustomVersionContainer.h"
#include "../../UE4/Versions/FPackageFileVersion.h"
#include "../../Utils/StringComparer.h"

namespace CUE4Parse::MappingsProvider::Usmap
{
    using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersionContainer;
    using CUE4Parse::UE4::Versions::FPackageFileVersion;

    // C#'s UsmapArchiveExtensions: a name-LUT index read (int32; -1 = no name).
    int32_t ReadNameEntry(FArchive& Ar);
    // The LUT entry for the next index, or nullopt for the invalid index.
    std::optional<std::string> ReadName(FArchive& Ar, const std::vector<std::string>& nameLut);

    // C#'s UsmapProperties statics.
    std::shared_ptr<Struct> ParseStruct(TypeMappings& context, FUsmapReader& Ar, const std::vector<std::string>& nameLut);
    std::shared_ptr<PropertyInfo> ParsePropertyInfo(FUsmapReader& Ar, const std::vector<std::string>& nameLut);
    std::shared_ptr<PropertyType> ParsePropertyType(FUsmapReader& Ar, const std::vector<std::string>& nameLut);

    class UsmapParser
    {
    public:
        static constexpr uint16_t FileMagic = 0x30C4;

        std::shared_ptr<TypeMappings> Mappings;
        EUsmapCompressionMethod CompressionMethod = EUsmapCompressionMethod::None;
        EUsmapVersion Version = EUsmapVersion::Initial;
        FPackageFileVersion PackageVersion;
        FCustomVersionContainer CustomVersions;
        uint32_t NetCL = 0;

        explicit UsmapParser(const std::string& path,
                             std::optional<Utils::StringComparer> comparer = std::nullopt);
        explicit UsmapParser(FArchive& archive,
                             std::optional<Utils::StringComparer> comparer = std::nullopt);

    private:
        void Parse(FArchive& archive, std::optional<Utils::StringComparer> comparer);
    };
}
