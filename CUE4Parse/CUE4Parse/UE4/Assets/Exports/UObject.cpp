// Ported from CUE4Parse/UE4/Assets/Exports/UObject.cs (Deserialize + tagged/unversioned property reading).
#include "UObject.h"

#include <memory>

#include "../IPackage.h"
#include "../ResolvedObject.h"
#include "../Readers/FAssetArchive.h"
#include "../Objects/Unversioned/FIterator.h"
#include "../Objects/Unversioned/FUnversionedHeader.h"
#include "../../Objects/UObject/UStruct.h"
#include "../../Objects/UObject/UScriptClass.h"
#include "../../Versions/ObjectVersion.h"
#include "../../Versions/EGame.h"
#include "../../Exceptions/ParserException.h"
#include "../../../MappingsProvider/MappingsSchema.h"

namespace CUE4Parse::UE4::Assets::Exports
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::UE4::Assets::Objects::Unversioned::FUnversionedHeader;
    using CUE4Parse::UE4::Assets::Objects::Unversioned::FIterator;
    using CUE4Parse::UE4::Assets::Objects::Properties::ReadType;

    void UObject::DeserializePropertiesTagged(std::vector<FPropertyTag>& properties, Readers::FAssetArchive& Ar, bool isStruct)
    {
        if (!isStruct && Ar.Ver() >= EUnrealEngineObjectUE5Version::PROPERTY_TAG_EXTENSION_AND_OVERRIDABLE_SERIALIZATION)
        {
            const auto serializationControl = Ar.Read<uint8_t>(); // EClassSerializationControlExtension
            if (serializationControl & 0x02 /* OverridableSerializationInformation */)
                Ar.Read<uint8_t>(); // Operation
        }

        while (true)
        {
            FPropertyTag tag(Ar, true);
            if (tag.Name.IsNone())
                break;
            properties.push_back(std::move(tag));
        }
    }

    void UObject::DeserializePropertiesUnversioned(std::vector<FPropertyTag>& properties, Readers::FAssetArchive& Ar,
                                                   const CUE4Parse::UE4::Objects::UObject::UStruct& struc)
    {
        const FUnversionedHeader header(Ar);
        if (!header.HasValues())
            return;
        const std::string& type = struc.Name;

        // C#: a UScriptClass (native class known by name) resolves through the mappings table; a really
        // loaded UStruct becomes a SerializedStruct over its FProperty children.
        const CUE4Parse::MappingsProvider::TypeMappings* mappings = Ar.Owner != nullptr ? Ar.Owner->Mappings() : nullptr;
        std::unique_ptr<CUE4Parse::MappingsProvider::SerializedStruct> serialized;
        const CUE4Parse::MappingsProvider::Struct* propMappings = nullptr;
        if (dynamic_cast<const CUE4Parse::UE4::Objects::UObject::UScriptClass*>(&struc) != nullptr)
        {
            if (mappings != nullptr)
            {
                const auto it = mappings->Types.find(type);
                if (it != mappings->Types.end())
                    propMappings = it->second.get();
            }
        }
        else
        {
            serialized = std::make_unique<CUE4Parse::MappingsProvider::SerializedStruct>(mappings, struc);
            propMappings = serialized.get();
        }

        if (propMappings == nullptr)
            throw Exceptions::ParserException(Ar, "Missing prop mappings for type " + type);

        FIterator it(header);
        do
        {
            const auto [val, isNonZero] = it.Current();
            if (isNonZero)
            {
                // The value has content and needs to be serialized normally.
                if (const auto* propertyInfo = propMappings->TryGetValue(val))
                {
                    FPropertyTag tag(Ar, *propertyInfo, ReadType::NORMAL);
                    if (tag.Tag != nullptr)
                        properties.push_back(std::move(tag));
                    else
                        throw Exceptions::ParserException(Ar,
                            type + ": Failed to serialize property " + propertyInfo->MappingType->Type + " " + propertyInfo->Name +
                            ". Can't proceed with serialization (Serialized " + std::to_string(properties.size()) + " properties until now)");
                }
                else
                {
                    throw Exceptions::ParserException(Ar,
                        type + ": Unknown property with value " + std::to_string(val) +
                        ". Can't proceed with serialization (Serialized " + std::to_string(properties.size()) + " properties until now)");
                }
            }
            else
            {
                // The value is serialized as zero, so no bytes are read here.
                if (const auto* propertyInfo = propMappings->TryGetValue(val))
                {
                    properties.emplace_back(Ar, *propertyInfo, ReadType::ZERO);
                }
                // C# logs "Unknown property with value {0} but it's zero so we are good".
            }
        } while (it.MoveNext());
    }

    void UObject::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        if (Ar.HasUnversionedProperties())
        {
            if (Class == nullptr)
                throw Exceptions::ParserException(Ar, "Found unversioned properties but object does not have a class");
            const auto* struc = dynamic_cast<const CUE4Parse::UE4::Objects::UObject::UStruct*>(Class->Object());
            if (struc == nullptr)
                throw Exceptions::ParserException(Ar, "Found unversioned properties but object's class is not a struct");

            Properties.clear();
            DeserializePropertiesUnversioned(Properties, Ar, *struc);
        }
        else
        {
            Properties.clear();
            DeserializePropertiesTagged(Properties, Ar, false);
        }

        if (Ar.Game() >= GAME_UE4_0 && !(Flags & RF_ClassDefaultObject))
        {
            const bool hasGuid = Ar.ReadBoolean();
            if (hasGuid)
            {
                if (Ar.Position + 16 > validPos)
                    throw Exceptions::ParserException(Ar, "Unexpected EOF in ObjectGuid");
                ObjectGuid = Ar.Read<FGuid>();
            }
        }

        // SparseClassData (UE5 BlueprintGeneratedClass) handling is deferred.
    }
}
