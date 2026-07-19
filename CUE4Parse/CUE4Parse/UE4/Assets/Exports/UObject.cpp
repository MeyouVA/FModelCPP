// Ported from CUE4Parse/UE4/Assets/Exports/UObject.cs (Deserialize + DeserializePropertiesTagged).
#include "UObject.h"

#include "../Readers/FAssetArchive.h"
#include "../../Versions/ObjectVersion.h"
#include "../../Versions/EGame.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Exports
{
    using namespace CUE4Parse::UE4::Versions;

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

    void UObject::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        if (Ar.HasUnversionedProperties())
            throw Exceptions::ParserException(Ar, "Unversioned properties are not yet supported");

        Properties.clear();
        DeserializePropertiesTagged(Properties, Ar, false);

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
