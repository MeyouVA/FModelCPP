// Ported from CUE4Parse/UE4/Assets/Objects/FPropertyTagData.cs (classic FAssetArchive ctor + ToString).
#include "FPropertyTagData.h"

#include "../Readers/FAssetArchive.h"
#include "../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using namespace CUE4Parse::UE4::Versions;

    FPropertyTagData::FPropertyTagData(Readers::FAssetArchive& Ar, const std::string& type, const std::string& name)
        : Name(name), Type(type)
    {
        if (type == "StructProperty")
        {
            StructType = Ar.ReadFName().Text();
            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::STRUCT_GUID_IN_PROPERTY_TAG)
                StructGuid = Ar.Read<FGuid>();
        }
        else if (type == "BoolProperty")
        {
            Bool = Ar.Ver() >= EUnrealEngineObjectUE3Version::PROPERTYTAG_BOOL_OPTIMIZATION ? Ar.ReadFlag() : Ar.ReadBoolean();
        }
        else if (type == "ByteProperty" || type == "EnumProperty")
        {
            if (Ar.Ver() >= EUnrealEngineObjectUE3Version::BYTEPROP_SERIALIZE_ENUM)
                EnumName = Ar.ReadFName().Text();
        }
        else if (type == "ArrayProperty")
        {
            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::ARRAY_PROPERTY_INNER_TAGS)
                InnerType = Ar.ReadFName().Text();
        }
        else if (type == "SetProperty")
        {
            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::PROPERTY_TAG_SET_MAP_SUPPORT)
                InnerType = Ar.ReadFName().Text();
        }
        else if (type == "MapProperty")
        {
            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::PROPERTY_TAG_SET_MAP_SUPPORT)
            {
                InnerType = Ar.ReadFName().Text();
                ValueType = Ar.ReadFName().Text();
            }
        }
        else if (type == "OptionalProperty")
        {
            InnerType = Ar.ReadFName().Text();
        }
    }

    std::string FPropertyTagData::ToString() const
    {
        std::string sb = Type;
        if (Type == "StructProperty")
            sb += "<" + StructType.value_or("") + ">";
        else if ((Type == "ByteProperty" && EnumName.has_value()) || Type == "EnumProperty")
            sb += "<" + EnumName.value_or("") + ">";
        else if (Type == "ArrayProperty" || Type == "SetProperty" || Type == "OptionalProperty")
            sb += "<" + InnerType.value_or("") + ">";
        else if (Type == "MapProperty")
            sb += "<" + InnerType.value_or("") + ", " + ValueType.value_or("") + ">";
        return sb;
    }
}
