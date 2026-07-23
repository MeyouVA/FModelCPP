// Ported from CUE4Parse/MappingsProvider/Usmap/UsmapParser.cs, UsmapProperties.cs and
// UsmapArchiveExtensions.cs.
#include "UsmapParser.h"

#include <fstream>

#include "../../Compression/Compression.h"
#include "../../Compression/CompressionMethod.h"
#include "../../UE4/Exceptions/ParserException.h"
#include "../../UE4/Readers/FByteArchive.h"

namespace CUE4Parse::MappingsProvider::Usmap
{
    using CUE4Parse::UE4::Exceptions::ParserException;
    using CUE4Parse::UE4::Readers::FByteArchive;
    using CUE4Parse::UE4::Objects::Core::Serialization::ECustomVersionSerializationFormat;
    using CompMethod = CUE4Parse::Compression::CompressionMethod;

    const char* PropertyTypeName(EPropertyType type)
    {
        switch (type)
        {
            case EPropertyType::ByteProperty: return "ByteProperty";
            case EPropertyType::BoolProperty: return "BoolProperty";
            case EPropertyType::IntProperty: return "IntProperty";
            case EPropertyType::FloatProperty: return "FloatProperty";
            case EPropertyType::ObjectProperty: return "ObjectProperty";
            case EPropertyType::NameProperty: return "NameProperty";
            case EPropertyType::DelegateProperty: return "DelegateProperty";
            case EPropertyType::DoubleProperty: return "DoubleProperty";
            case EPropertyType::ArrayProperty: return "ArrayProperty";
            case EPropertyType::StructProperty: return "StructProperty";
            case EPropertyType::StrProperty: return "StrProperty";
            case EPropertyType::TextProperty: return "TextProperty";
            case EPropertyType::InterfaceProperty: return "InterfaceProperty";
            case EPropertyType::MulticastDelegateProperty: return "MulticastDelegateProperty";
            case EPropertyType::WeakObjectProperty: return "WeakObjectProperty";
            case EPropertyType::LazyObjectProperty: return "LazyObjectProperty";
            case EPropertyType::AssetObjectProperty: return "AssetObjectProperty";
            case EPropertyType::SoftObjectProperty: return "SoftObjectProperty";
            case EPropertyType::UInt64Property: return "UInt64Property";
            case EPropertyType::UInt32Property: return "UInt32Property";
            case EPropertyType::UInt16Property: return "UInt16Property";
            case EPropertyType::Int64Property: return "Int64Property";
            case EPropertyType::Int16Property: return "Int16Property";
            case EPropertyType::Int8Property: return "Int8Property";
            case EPropertyType::MapProperty: return "MapProperty";
            case EPropertyType::SetProperty: return "SetProperty";
            case EPropertyType::EnumProperty: return "EnumProperty";
            case EPropertyType::FieldPathProperty: return "FieldPathProperty";
            case EPropertyType::OptionalProperty: return "OptionalProperty";
            case EPropertyType::Utf8StrProperty: return "Utf8StrProperty";
            case EPropertyType::AnsiStrProperty: return "AnsiStrProperty";
            case EPropertyType::ClassProperty: return "ClassProperty";
            case EPropertyType::MulticastInlineDelegateProperty: return "MulticastInlineDelegateProperty";
            case EPropertyType::SoftClassProperty: return "SoftClassProperty";
            case EPropertyType::VerseStringProperty: return "VerseStringProperty";
            case EPropertyType::VerseDynamicProperty: return "VerseDynamicProperty";
            case EPropertyType::VerseFunctionProperty: return "VerseFunctionProperty";
            case EPropertyType::CustomProperty_FD: return "CustomProperty_FD";
            case EPropertyType::CustomProperty_FE: return "CustomProperty_FE";
            case EPropertyType::Unknown: return "Unknown";
            default: return ""; // C#: Enum.GetName -> null -> string.Empty
        }
    }

    int32_t ReadNameEntry(FArchive& Ar)
    {
        return Ar.Read<int32_t>();
    }

    std::optional<std::string> ReadName(FArchive& Ar, const std::vector<std::string>& nameLut)
    {
        constexpr int32_t InvalidNameIndex = -1;
        const int32_t idx = ReadNameEntry(Ar);
        if (idx == InvalidNameIndex) return std::nullopt;
        if (idx < 0 || static_cast<size_t>(idx) >= nameLut.size())
            throw ParserException("usmap name index " + std::to_string(idx) + " out of range (" +
                                  std::to_string(nameLut.size()) + " names) at " + std::to_string(Ar.Position));
        return nameLut[static_cast<size_t>(idx)];
    }

    std::shared_ptr<PropertyType> ParsePropertyType(FUsmapReader& Ar, const std::vector<std::string>& nameLut)
    {
        const auto typeEnum = static_cast<EPropertyType>(Ar.Read<uint8_t>());
        std::string type = PropertyTypeName(typeEnum);
        std::optional<std::string> structType;
        std::shared_ptr<PropertyType> innerType;
        std::shared_ptr<PropertyType> valueType;
        std::optional<std::string> enumName;
        const std::optional<bool> isEnumAsByte = std::nullopt;

        switch (typeEnum)
        {
            case EPropertyType::EnumProperty:
                innerType = ParsePropertyType(Ar, nameLut);
                enumName = ReadName(Ar, nameLut);
                break;
            case EPropertyType::StructProperty:
                structType = ReadName(Ar, nameLut);
                break;
            case EPropertyType::SetProperty:
            case EPropertyType::ArrayProperty:
            case EPropertyType::OptionalProperty:
                innerType = ParsePropertyType(Ar, nameLut);
                break;
            case EPropertyType::MapProperty:
                innerType = ParsePropertyType(Ar, nameLut);
                valueType = ParsePropertyType(Ar, nameLut);
                break;
            default:
                break;
        }

        return std::make_shared<PropertyType>(std::move(type), std::move(structType), std::move(innerType),
                                              std::move(valueType), std::move(enumName), isEnumAsByte);
    }

    std::shared_ptr<PropertyInfo> ParsePropertyInfo(FUsmapReader& Ar, const std::vector<std::string>& nameLut)
    {
        const auto index = Ar.Read<uint16_t>();
        const auto arrayDim = Ar.Read<uint8_t>();
        auto name = ReadName(Ar, nameLut);
        auto type = ParsePropertyType(Ar, nameLut);
        return std::make_shared<PropertyInfo>(index, name.value_or(std::string()), std::move(type), arrayDim);
    }

    std::shared_ptr<Struct> ParseStruct(TypeMappings& context, FUsmapReader& Ar, const std::vector<std::string>& nameLut)
    {
        auto name = ReadName(Ar, nameLut);
        auto superType = ReadName(Ar, nameLut);

        const auto propertyCount = Ar.Read<uint16_t>();
        const auto serializablePropertyCount = Ar.Read<uint16_t>();
        std::map<int, std::shared_ptr<PropertyInfo>> properties;
        for (int i = 0; i < serializablePropertyCount; i++)
        {
            const auto propInfo = ParsePropertyInfo(Ar, nameLut);
            const int arraySize = propInfo->ArraySize.value_or(1);
            for (int j = 0; j < arraySize; j++)
            {
                // C#'s (PropertyInfo) propInfo.Clone() with Index = j (a shallow copy; MappingType shared).
                auto clone = std::make_shared<PropertyInfo>(*propInfo);
                clone->Index = j;
                properties[propInfo->Index + j] = std::move(clone);
            }
        }

        return std::make_shared<Struct>(&context, name.value_or(std::string()), std::move(superType),
                                        std::move(properties), propertyCount);
    }

    UsmapParser::UsmapParser(const std::string& path, std::optional<Utils::StringComparer> comparer)
    {
        // C# opens a FileStream (FStreamArchive); this port reads the file into an FByteArchive.
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw ParserException("Couldn't open usmap file: " + path);
        const auto size = static_cast<std::streamsize>(file.tellg());
        std::vector<uint8_t> bytes(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);
        if (size > 0) file.read(reinterpret_cast<char*>(bytes.data()), size);

        FByteArchive archive(path, std::move(bytes));
        Parse(archive, comparer);
    }

    UsmapParser::UsmapParser(FArchive& archive, std::optional<Utils::StringComparer> comparer)
    {
        Parse(archive, comparer);
    }

    void UsmapParser::Parse(FArchive& archive, std::optional<Utils::StringComparer> comparer)
    {
        if (archive.Length < 2)
            throw ParserException("Usmap is empty");

        const auto magic = archive.Read<uint16_t>();
        if (magic != FileMagic)
            throw ParserException("Usmap has invalid magic");

        Version = static_cast<EUsmapVersion>(archive.Read<uint8_t>());
        if (Version > EUsmapVersion::Latest)
            throw ParserException("Usmap has invalid version (" + std::to_string(static_cast<int>(Version)) + ")");

        FUsmapReader Ar(archive, Version);

        const bool bHasVersioning = Ar.Version >= EUsmapVersion::PackageVersioning && Ar.ReadBoolean();
        if (bHasVersioning)
        {
            // Sequenced through locals: C# reads the UE4 version then the UE5 one, and C++ leaves
            // argument evaluation order unspecified (MSVC evaluates right-to-left, so the inline form
            // silently swapped them).
            const auto ue4Version = Ar.Read<int32_t>();
            const auto ue5Version = Ar.Read<int32_t>();
            PackageVersion = FPackageFileVersion(ue4Version, ue5Version);
            CustomVersions = FCustomVersionContainer(Ar, ECustomVersionSerializationFormat::Latest);
            NetCL = Ar.Read<uint32_t>();
        }
        else
        {
            PackageVersion = FPackageFileVersion();
            CustomVersions = FCustomVersionContainer();
            NetCL = 0;
        }

        CompressionMethod = static_cast<EUsmapCompressionMethod>(Ar.Read<uint8_t>());

        const auto compSize = Ar.Read<uint32_t>();
        const auto decompSize = Ar.Read<uint32_t>();

        std::vector<uint8_t> data;

        if (CompressionMethod == EUsmapCompressionMethod::None)
        {
            if (compSize != decompSize)
                throw ParserException("No compression: Compression size must be equal to decompression size");
            data = Ar.ReadBytes(static_cast<int>(compSize));
        }
        else
        {
            CompMethod method;
            switch (CompressionMethod)
            {
                case EUsmapCompressionMethod::Oodle: method = CompMethod::Oodle; break;
                case EUsmapCompressionMethod::Brotli: method = CompMethod::Brotli; break;
                case EUsmapCompressionMethod::ZStandard: method = CompMethod::Zstd; break;
                default: method = CompMethod::Unknown; break;
            }
            const auto compressed = Ar.ReadBytes(static_cast<int>(compSize));
            data = Compression::Compression::Decompress(compressed, static_cast<int>(decompSize), method, &Ar);
        }

        FByteArchive bodyArchive(Ar.Name(), std::move(data), Ar.Versions);
        FUsmapReader body(bodyArchive, Ar.Version);

        const auto nameSize = body.Read<uint32_t>();
        std::vector<std::string> nameLut;
        nameLut.reserve(nameSize);
        for (uint32_t i = 0; i < nameSize; i++)
        {
            const int nameLength = body.Version >= EUsmapVersion::LongFName
                ? static_cast<int>(body.Read<uint16_t>())
                : static_cast<int>(body.Read<uint8_t>());
            const auto bytes = body.ReadBytes(nameLength);
            nameLut.emplace_back(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }

        const auto enumCount = body.Read<uint32_t>();
        std::map<std::string, std::map<int64_t, std::string>> enums;
        for (uint32_t i = 0; i < enumCount; i++)
        {
            auto enumName = ReadName(body, nameLut).value_or(std::string());

            const int enumNamesSize = body.Version >= EUsmapVersion::LargeEnums
                ? static_cast<int>(body.Read<uint16_t>())
                : static_cast<int>(body.Read<uint8_t>());
            std::map<int64_t, std::string> enumNames;

            if (body.Version >= EUsmapVersion::ExplicitEnumValues)
            {
                for (int j = 0; j < enumNamesSize; j++)
                {
                    const auto value = body.Read<uint64_t>();
                    enumNames[static_cast<int64_t>(value)] = ReadName(body, nameLut).value_or(std::string());
                }
            }
            else
            {
                for (int j = 0; j < enumNamesSize; j++)
                {
                    enumNames[j] = ReadName(body, nameLut).value_or(std::string());
                }
            }

            // C#'s TryAdd: duplicated enums (some games ship them, even with different values) are ignored.
            enums.emplace(std::move(enumName), std::move(enumNames));
        }

        const auto structCount = body.Read<uint32_t>();
        Mappings = std::make_shared<TypeMappings>(comparer.value_or(Utils::StringComparer::OrdinalIgnoreCase()));
        Mappings->Enums = std::move(enums);

        for (uint32_t i = 0; i < structCount; i++)
        {
            auto s = ParseStruct(*Mappings, body, nameLut);
            Mappings->Types.insert_or_assign(s->Name, std::move(s));
        }
    }
}
