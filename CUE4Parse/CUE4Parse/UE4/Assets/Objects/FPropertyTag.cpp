// Ported from CUE4Parse/UE4/Assets/Objects/FPropertyTag.cs (classic tag ctor).
#include "FPropertyTag.h"

#include "../Readers/FAssetArchive.h"
#include "../../Versions/ObjectVersion.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using namespace CUE4Parse::UE4::Versions;

    FPropertyTag::FPropertyTag(Readers::FAssetArchive& Ar, bool readData)
    {
        Name = Ar.ReadFName();
        if (Name.IsNone())
            return;

        if (Ar.Ver() >= EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME)
        {
            // UE5 complete-type-name (FPropertyTypeNameNode) layout is not ported yet.
            throw Exceptions::ParserException(Ar, "UE5 PROPERTY_TAG_COMPLETE_TYPE_NAME property tags are not yet supported");
        }

        PropertyType = Ar.ReadFName();
        Size = Ar.Read<int32_t>();
        ArrayIndex = Ar.Read<int32_t>();
        TagData = std::make_unique<FPropertyTagData>(Ar, PropertyType.Text(), Name.Text());

        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::PROPERTY_GUID_IN_PROPERTY_TAG)
        {
            HasPropertyGuid = Ar.ReadFlag();
            if (HasPropertyGuid)
                PropertyGuid = Ar.Read<FGuid>();
        }

        if (Ar.Ver() >= EUnrealEngineObjectUE5Version::PROPERTY_TAG_EXTENSION_AND_OVERRIDABLE_SERIALIZATION)
        {
            const auto tagExtensions = Ar.Read<uint8_t>(); // EPropertyTagExtension
            if (tagExtensions & 0x02 /* OverridableInformation */)
            {
                Ar.Read<uint8_t>();  // EOverriddenPropertyOperation
                Ar.ReadBoolean();    // bExperimentalOverridableLogic
            }
        }

        if (!readData)
            return;

        const int64_t pos = Ar.Position;
        const int64_t finalPos = pos + Size;
        try
        {
            Tag = FPropertyTagType::ReadPropertyTagType(Ar, PropertyType.Text(), TagData.get(), Properties::ReadType::NORMAL, Size);
        }
        catch (const Exceptions::ParserException&)
        {
            // Swallow: always seek to the calculated position below, no need to crash on one bad property.
        }
        // Always seek to the calculated end position.
        Ar.Position = finalPos;
    }
}
