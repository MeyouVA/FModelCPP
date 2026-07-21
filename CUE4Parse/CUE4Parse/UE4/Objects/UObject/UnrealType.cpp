// Ported from CUE4Parse/UE4/Objects/UObject/UnrealType.cs (+ FField::Construct / SerializeSingleField).
#include "UnrealType.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    namespace
    {
        // C#'s `(FProperty?) SerializeSingleField(Ar)` — null if the read field isn't an FProperty.
        std::unique_ptr<FProperty> ReadProperty(FAssetArchive& Ar)
        {
            auto field = FField::SerializeSingleField(Ar);
            if (field && dynamic_cast<FProperty*>(field.get()))
                return std::unique_ptr<FProperty>(static_cast<FProperty*>(field.release()));
            return nullptr;
        }

        std::unique_ptr<FNumericProperty> ReadNumeric(FAssetArchive& Ar)
        {
            auto field = FField::SerializeSingleField(Ar);
            if (field && dynamic_cast<FNumericProperty*>(field.get()))
                return std::unique_ptr<FNumericProperty>(static_cast<FNumericProperty*>(field.release()));
            return nullptr;
        }
    }

    void FProperty::Deserialize(FAssetArchive& Ar)
    {
        FField::Deserialize(Ar);
        ArrayDim = Ar.Read<int32_t>();
        ElementSize = Ar.Read<int32_t>();
        PropertyFlags = Ar.Read<EPropertyFlags>();
        RepIndex = Ar.Read<uint16_t>();
        RepNotifyFunc = Ar.ReadFName();
        BlueprintReplicationCondition = static_cast<ELifetimeCondition>(Ar.Read<uint8_t>());
    }

    void FArrayProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        Inner = ReadProperty(Ar);
    }

    void FBoolProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        FieldSize = Ar.Read<uint8_t>();
        ByteOffset = Ar.Read<uint8_t>();
        ByteMask = Ar.Read<uint8_t>();
        FieldMask = Ar.Read<uint8_t>();
        BoolSize = Ar.Read<uint8_t>();
        bIsNativeBool = Ar.ReadFlag();
    }

    void FByteProperty::Deserialize(FAssetArchive& Ar)
    {
        FNumericProperty::Deserialize(Ar);
        Enum = FPackageIndex(Ar);
    }

    void FObjectProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        PropertyClass = FPackageIndex(Ar);
    }

    void FClassProperty::Deserialize(FAssetArchive& Ar)
    {
        FObjectProperty::Deserialize(Ar);
        MetaClass = FPackageIndex(Ar);
    }

    void FDelegateProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        SignatureFunction = FPackageIndex(Ar);
    }

    void FEnumProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        Enum = FPackageIndex(Ar);
        UnderlyingProp = ReadNumeric(Ar);
    }

    void FFieldPathProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        PropertyClass = Ar.ReadFName();
    }

    void FInterfaceProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        InterfaceClass = FPackageIndex(Ar);
    }

    void FMapProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        KeyProp = ReadProperty(Ar);
        ValueProp = ReadProperty(Ar);
    }

    void FMulticastDelegateProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        SignatureFunction = FPackageIndex(Ar);
    }

    void FMulticastInlineDelegateProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        SignatureFunction = FPackageIndex(Ar);
    }

    void FSetProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        ElementProp = ReadProperty(Ar);
    }

    void FStructProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        Struct = FPackageIndex(Ar);
    }

    void FOptionalProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        ValueProperty = ReadProperty(Ar);
    }

    void FVerseStringProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        ValueProperty = ReadProperty(Ar);
    }

    void FVerseFunctionProperty::Deserialize(FAssetArchive& Ar)
    {
        FProperty::Deserialize(Ar);
        Function = FPackageIndex(Ar);
    }

    void FVerseClassProperty::Deserialize(FAssetArchive& Ar)
    {
        FClassProperty::Deserialize(Ar);
        bRequiresConcrete = Ar.ReadBoolean();
        bRequiresCastable = Ar.ReadBoolean();
    }

    // --- FField factory (defined here where the concrete subclasses are complete types) ---

    std::unique_ptr<FField> FField::Construct(const FName& fieldTypeName)
    {
        const std::string t = fieldTypeName.Text();
        if (t == "ArrayProperty")                    return std::make_unique<FArrayProperty>();
        if (t == "BoolProperty")                     return std::make_unique<FBoolProperty>();
        if (t == "ByteProperty")                     return std::make_unique<FByteProperty>();
        if (t == "ClassProperty")                    return std::make_unique<FClassProperty>();
        if (t == "DelegateProperty")                 return std::make_unique<FDelegateProperty>();
        if (t == "EnumProperty")                     return std::make_unique<FEnumProperty>();
        if (t == "FieldPathProperty")                return std::make_unique<FFieldPathProperty>();
        if (t == "DoubleProperty")                   return std::make_unique<FDoubleProperty>();
        if (t == "FloatProperty")                    return std::make_unique<FFloatProperty>();
        if (t == "Int16Property")                    return std::make_unique<FInt16Property>();
        if (t == "Int64Property")                    return std::make_unique<FInt64Property>();
        if (t == "Int8Property")                     return std::make_unique<FInt8Property>();
        if (t == "IntProperty")                      return std::make_unique<FIntProperty>();
        if (t == "InterfaceProperty")                return std::make_unique<FInterfaceProperty>();
        if (t == "MapProperty")                      return std::make_unique<FMapProperty>();
        if (t == "MulticastDelegateProperty")        return std::make_unique<FMulticastDelegateProperty>();
        if (t == "MulticastInlineDelegateProperty")  return std::make_unique<FMulticastInlineDelegateProperty>();
        if (t == "NameProperty")                     return std::make_unique<FNameProperty>();
        if (t == "ObjectProperty" || t == "ObjectPtrProperty") return std::make_unique<FObjectProperty>();
        if (t == "SetProperty")                      return std::make_unique<FSetProperty>();
        if (t == "SoftClassProperty")                return std::make_unique<FSoftClassProperty>();
        if (t == "SoftObjectProperty")               return std::make_unique<FSoftObjectProperty>();
        if (t == "StrProperty")                      return std::make_unique<FStrProperty>();
        if (t == "Utf8StrProperty")                  return std::make_unique<FUtf8StrProperty>();
        if (t == "StructProperty")                   return std::make_unique<FStructProperty>();
        if (t == "TextProperty")                     return std::make_unique<FTextProperty>();
        if (t == "UInt16Property")                   return std::make_unique<FUInt16Property>();
        if (t == "UInt32Property")                   return std::make_unique<FUInt32Property>();
        if (t == "UInt64Property")                   return std::make_unique<FUInt64Property>();
        if (t == "WeakObjectProperty")               return std::make_unique<FWeakObjectProperty>();
        if (t == "OptionalProperty")                 return std::make_unique<FOptionalProperty>();
        if (t == "VerseStringProperty")              return std::make_unique<FVerseStringProperty>();
        if (t == "VerseClassProperty")               return std::make_unique<FVerseClassProperty>();
        if (t == "VerseFunctionProperty")            return std::make_unique<FVerseFunctionProperty>();
        if (t == "VerseDynamicProperty")             return std::make_unique<FVerseDynamicProperty>();
        if (t == "ReferenceProperty")                return std::make_unique<FReferenceProperty>();
        // Borderlands4 game-specific field types (GameDataHandleProperty / GbxDefPtrProperty) are not ported.
        throw Exceptions::ParserException("Unsupported serialized property type " + t);
    }

    std::unique_ptr<FField> FField::SerializeSingleField(FAssetArchive& Ar)
    {
        const FName propertyTypeName = Ar.ReadFName();
        if (propertyTypeName.Text() != "None")
        {
            auto field = Construct(propertyTypeName);
            field->Deserialize(Ar);
            return field;
        }
        return nullptr;
    }
}
