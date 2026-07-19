// Ported from CUE4Parse/UE4/Assets/Objects/Properties/FPropertyTagType.cs (factory + value formatters).
#include "FPropertyTagType.h"

#include <cstdio>

#include "../FPropertyTagData.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/FName.h"

#include "BoolProperty.h"
#include "ByteProperty.h"
#include "Int8Property.h"
#include "Int16Property.h"
#include "IntProperty.h"
#include "Int64Property.h"
#include "UInt16Property.h"
#include "UInt32Property.h"
#include "UInt64Property.h"
#include "FloatProperty.h"
#include "DoubleProperty.h"
#include "NameProperty.h"
#include "StrProperty.h"
#include "EnumProperty.h"
#include "StructProperty.h"
#include "ArrayProperty.h"
#include "MapProperty.h"
#include "SetProperty.h"
#include "ObjectProperty.h"
#include "WeakObjectProperty.h"
#include "ClassProperty.h"
#include "AssetObjectProperty.h"
#include "LazyObjectProperty.h"
#include "SoftObjectProperty.h"
#include "DelegateProperty.h"
#include "MulticastDelegateProperty.h"
#include "InterfaceProperty.h"
#include "FieldPathProperty.h"
#include "AnsiStrProperty.h"
#include "Utf8StrProperty.h"
#include "VerseStringProperty.h"
#include "TextProperty.h"
#include "OptionalProperty.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    std::string PropValueToString(bool v) { return v ? "true" : "false"; }
    std::string PropValueToString(signed char v) { return std::to_string(static_cast<int>(v)); }
    std::string PropValueToString(unsigned char v) { return std::to_string(static_cast<unsigned>(v)); }
    std::string PropValueToString(short v) { return std::to_string(v); }
    std::string PropValueToString(unsigned short v) { return std::to_string(v); }
    std::string PropValueToString(int v) { return std::to_string(v); }
    std::string PropValueToString(unsigned int v) { return std::to_string(v); }
    std::string PropValueToString(long long v) { return std::to_string(v); }
    std::string PropValueToString(unsigned long long v) { return std::to_string(v); }
    std::string PropValueToString(float v) { char b[32]; std::snprintf(b, sizeof(b), "%g", static_cast<double>(v)); return b; }
    std::string PropValueToString(double v) { char b[32]; std::snprintf(b, sizeof(b), "%g", v); return b; }
    std::string PropValueToString(const std::string& v) { return v; }
    std::string PropValueToString(const CUE4Parse::UE4::Objects::UObject::FName& v) { return v.Text(); }

    std::unique_ptr<FPropertyTagType> FPropertyTagType::ReadPropertyTagType(
        FAssetArchive& Ar, const std::string& propertyType, const FPropertyTagData* tagData, ReadType type, int size)
    {
        // The full tagged-property value set: scalars + Struct/Array/Enum/Map/Set/Object + Soft/object-ish +
        // Delegate/Interface/FieldPath + Ansi/Utf8/Verse strings + Text + Optional. Only game-specific readers
        // (Borderlands4 / OuterWorlds2 / etc.) remain unported -> null.
        if (propertyType == "BoolProperty")   return std::make_unique<BoolProperty>(Ar, tagData, type);
        if (propertyType == "ByteProperty")   return std::make_unique<ByteProperty>(Ar, type);
        if (propertyType == "Int8Property")   return std::make_unique<Int8Property>(Ar, type);
        if (propertyType == "Int16Property")  return std::make_unique<Int16Property>(Ar, type);
        if (propertyType == "IntProperty")    return std::make_unique<IntProperty>(Ar, type);
        if (propertyType == "Int64Property")  return std::make_unique<Int64Property>(Ar, type);
        if (propertyType == "UInt16Property") return std::make_unique<UInt16Property>(Ar, type);
        if (propertyType == "UInt32Property") return std::make_unique<UInt32Property>(Ar, type);
        if (propertyType == "UInt64Property") return std::make_unique<UInt64Property>(Ar, type);
        if (propertyType == "FloatProperty")  return std::make_unique<FloatProperty>(Ar, type);
        if (propertyType == "DoubleProperty") return std::make_unique<DoubleProperty>(Ar, type);
        if (propertyType == "NameProperty")   return std::make_unique<NameProperty>(Ar, type);
        if (propertyType == "StrProperty")    return std::make_unique<StrProperty>(Ar, type);
        if (propertyType == "EnumProperty")   return std::make_unique<EnumProperty>(Ar, tagData, type);
        if (propertyType == "StructProperty") return std::make_unique<StructProperty>(Ar, tagData, type);
        if (propertyType == "ArrayProperty")  return std::make_unique<ArrayProperty>(Ar, tagData, type, size);
        if (propertyType == "MapProperty")    return std::make_unique<MapProperty>(Ar, tagData, type);
        if (propertyType == "SetProperty")    return std::make_unique<SetProperty>(Ar, tagData, type);
        if (propertyType == "ObjectProperty") return std::make_unique<ObjectProperty>(Ar, type);
        if (propertyType == "WeakObjectProperty") return std::make_unique<WeakObjectProperty>(Ar, type);
        if (propertyType == "ClassProperty")  return std::make_unique<ClassProperty>(Ar, type);
        if (propertyType == "LazyObjectProperty") return std::make_unique<LazyObjectProperty>(Ar, type);
        if (propertyType == "AssetObjectProperty" || propertyType == "AssetClassProperty")
            return std::make_unique<AssetObjectProperty>(Ar, type);
        if (propertyType == "SoftObjectProperty" || propertyType == "SoftClassProperty" || propertyType == "ReferenceProperty")
            return std::make_unique<SoftObjectProperty>(Ar, type);
        if (propertyType == "DelegateProperty" || propertyType == "VerseFunctionProperty")
            return std::make_unique<DelegateProperty>(Ar, type);
        if (propertyType == "MulticastDelegateProperty") return std::make_unique<MulticastDelegateProperty>(Ar, type);
        if (propertyType == "MulticastInlineDelegateProperty") return std::make_unique<MulticastInlineDelegateProperty>(Ar, type);
        if (propertyType == "MulticastSparseDelegateProperty") return std::make_unique<MulticastSparseDelegateProperty>(Ar, type);
        if (propertyType == "InterfaceProperty") return std::make_unique<InterfaceProperty>(Ar, type);
        if (propertyType == "FieldPathProperty") return std::make_unique<FieldPathProperty>(Ar, type);
        if (propertyType == "AnsiStrProperty") return std::make_unique<AnsiStrProperty>(Ar, type);
        if (propertyType == "Utf8StrProperty") return std::make_unique<Utf8StrProperty>(Ar, type);
        if (propertyType == "VerseStringProperty") return std::make_unique<VerseStringProperty>(Ar, type);
        if (propertyType == "TextProperty")     return std::make_unique<TextProperty>(Ar, type);
        if (propertyType == "OptionalProperty") return std::make_unique<OptionalProperty>(Ar, tagData, type);
        return nullptr;
    }
}
