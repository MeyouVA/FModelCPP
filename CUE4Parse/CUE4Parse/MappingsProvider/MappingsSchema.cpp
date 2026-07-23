// Ported from CUE4Parse/MappingsProvider/MappingsSchema.cs (out-of-line members).
#include "MappingsSchema.h"

#include <algorithm>

#include "../UE4/Assets/ResolvedObject.h"
#include "../UE4/Objects/UObject/UnrealType.h"
#include "../UE4/Objects/UObject/UStruct.h"
#include "../UE4/Objects/UObject/UEnum.h"
#include "../UE4/Objects/UObject/UScriptClass.h"

namespace CUE4Parse::MappingsProvider
{
    using namespace CUE4Parse::UE4::Objects::UObject;
    using CUE4Parse::UE4::Assets::ResolvedObject;

    namespace
    {
        // C#'s prop.GetType().Name[1..]: the concrete FProperty class name without the leading 'F'.
        // Most-derived types are tested before their bases.
        std::string PropTypeName(const FProperty& prop)
        {
            if (dynamic_cast<const FSoftClassProperty*>(&prop)) return "SoftClassProperty";
            if (dynamic_cast<const FVerseClassProperty*>(&prop)) return "VerseClassProperty";
            if (dynamic_cast<const FClassProperty*>(&prop)) return "ClassProperty";
            if (dynamic_cast<const FSoftObjectProperty*>(&prop)) return "SoftObjectProperty";
            if (dynamic_cast<const FWeakObjectProperty*>(&prop)) return "WeakObjectProperty";
            if (dynamic_cast<const FObjectProperty*>(&prop)) return "ObjectProperty";
            if (dynamic_cast<const FArrayProperty*>(&prop)) return "ArrayProperty";
            if (dynamic_cast<const FBoolProperty*>(&prop)) return "BoolProperty";
            if (dynamic_cast<const FByteProperty*>(&prop)) return "ByteProperty";
            if (dynamic_cast<const FDelegateProperty*>(&prop)) return "DelegateProperty";
            if (dynamic_cast<const FEnumProperty*>(&prop)) return "EnumProperty";
            if (dynamic_cast<const FFieldPathProperty*>(&prop)) return "FieldPathProperty";
            if (dynamic_cast<const FDoubleProperty*>(&prop)) return "DoubleProperty";
            if (dynamic_cast<const FFloatProperty*>(&prop)) return "FloatProperty";
            if (dynamic_cast<const FInt16Property*>(&prop)) return "Int16Property";
            if (dynamic_cast<const FInt64Property*>(&prop)) return "Int64Property";
            if (dynamic_cast<const FInt8Property*>(&prop)) return "Int8Property";
            if (dynamic_cast<const FIntProperty*>(&prop)) return "IntProperty";
            if (dynamic_cast<const FInterfaceProperty*>(&prop)) return "InterfaceProperty";
            if (dynamic_cast<const FMapProperty*>(&prop)) return "MapProperty";
            if (dynamic_cast<const FMulticastInlineDelegateProperty*>(&prop)) return "MulticastInlineDelegateProperty";
            if (dynamic_cast<const FMulticastDelegateProperty*>(&prop)) return "MulticastDelegateProperty";
            if (dynamic_cast<const FNameProperty*>(&prop)) return "NameProperty";
            if (dynamic_cast<const FSetProperty*>(&prop)) return "SetProperty";
            if (dynamic_cast<const FStrProperty*>(&prop)) return "StrProperty";
            if (dynamic_cast<const FUtf8StrProperty*>(&prop)) return "Utf8StrProperty";
            if (dynamic_cast<const FStructProperty*>(&prop)) return "StructProperty";
            if (dynamic_cast<const FTextProperty*>(&prop)) return "TextProperty";
            if (dynamic_cast<const FUInt16Property*>(&prop)) return "UInt16Property";
            if (dynamic_cast<const FUInt32Property*>(&prop)) return "UInt32Property";
            if (dynamic_cast<const FUInt64Property*>(&prop)) return "UInt64Property";
            if (dynamic_cast<const FOptionalProperty*>(&prop)) return "OptionalProperty";
            if (dynamic_cast<const FVerseStringProperty*>(&prop)) return "VerseStringProperty";
            if (dynamic_cast<const FVerseFunctionProperty*>(&prop)) return "VerseFunctionProperty";
            if (dynamic_cast<const FVerseDynamicProperty*>(&prop)) return "VerseDynamicProperty";
            if (dynamic_cast<const FReferenceProperty*>(&prop)) return "ReferenceProperty";
            if (dynamic_cast<const FNumericProperty*>(&prop)) return "NumericProperty";
            return "Property";
        }

        ResolvedObject* Resolve(const FPackageIndex& index)
        {
            return index.Owner != nullptr ? index.Owner->ResolvePackageIndex(&index) : nullptr;
        }
    }

    PropertyType::PropertyType(const FProperty& prop)
        : Type(PropTypeName(prop))
    {
        // C#'s ApplyEnum local (shared by the Byte/Enum arms below).
        const auto applyEnum = [this, &prop](const FPackageIndex& enumIndex)
        {
            ResolvedObject* enumObj = Resolve(enumIndex);
            if (enumObj != nullptr)
            {
                Enum = dynamic_cast<const UEnum*>(enumObj->Object());
                EnumName = enumObj->Name().Text();
            }
            InnerType = prop.ElementSize == 4 ? std::make_shared<PropertyType>("IntProperty") : nullptr;
        };

        if (const auto* array = dynamic_cast<const FArrayProperty*>(&prop))
        {
            if (array->Inner) InnerType = std::make_shared<PropertyType>(*array->Inner);
        }
        else if (const auto* b = dynamic_cast<const FByteProperty*>(&prop))
        {
            applyEnum(b->Enum);
        }
        else if (const auto* e = dynamic_cast<const FEnumProperty*>(&prop))
        {
            applyEnum(e->Enum);
        }
        else if (const auto* map = dynamic_cast<const FMapProperty*>(&prop))
        {
            if (map->KeyProp) InnerType = std::make_shared<PropertyType>(*map->KeyProp);
            if (map->ValueProp) ValueType = std::make_shared<PropertyType>(*map->ValueProp);
        }
        else if (const auto* set = dynamic_cast<const FSetProperty*>(&prop))
        {
            if (set->ElementProp) InnerType = std::make_shared<PropertyType>(*set->ElementProp);
        }
        else if (const auto* struc = dynamic_cast<const FStructProperty*>(&prop))
        {
            ResolvedObject* structObj = Resolve(struc->Struct);
            if (structObj != nullptr)
            {
                Struct = dynamic_cast<const UStruct*>(structObj->Object());
                StructType = structObj->Name().Text();
            }
        }
        else if (const auto* optional = dynamic_cast<const FOptionalProperty*>(&prop))
        {
            if (optional->ValueProperty) InnerType = std::make_shared<PropertyType>(*optional->ValueProperty);
        }
    }

    Struct* Struct::ResolveSuper() const
    {
        if (SuperType.has_value() && Context != nullptr)
        {
            const auto it = Context->Types.find(*SuperType);
            if (it != Context->Types.end())
                return it->second.get();
        }
        return nullptr;
    }

    SerializedStruct::SerializedStruct(const TypeMappings* context, const UStruct& struc)
        : Struct(context, struc.Name, static_cast<int>(struc.ChildProperties.size())), _struct(&struc)
    {
        for (size_t i = 0; i < struc.ChildProperties.size(); i++)
        {
            // C# hard-casts every child to FProperty; a non-property child would throw there, so skipping
            // one here can only be more permissive, never less.
            const auto* prop = dynamic_cast<const FProperty*>(struc.ChildProperties[i].get());
            if (prop == nullptr) continue;

            auto propInfo = std::make_shared<PropertyInfo>(
                std::min(static_cast<int>(i), prop->ArrayDim - 1), prop->Name.Text(),
                std::make_shared<PropertyType>(*prop), prop->ArrayDim);
            for (int j = 0; j < prop->ArrayDim; j++)
                Properties[static_cast<int>(i) + j] = propInfo;
        }
    }

    Struct* SerializedStruct::ResolveSuper() const
    {
        ResolvedObject* resolved = Resolve(_struct->SuperStruct);
        auto* superStruct = resolved != nullptr ? dynamic_cast<UStruct*>(resolved->Object()) : nullptr;
        if (superStruct != nullptr)
        {
            if (dynamic_cast<UScriptClass*>(superStruct) != nullptr)
            {
                if (Context != nullptr)
                {
                    const auto it = Context->Types.find(superStruct->Name);
                    if (it != Context->Types.end())
                        return it->second.get();
                }
                // C# logs "Missing prop mappings for type {0}" here.
                return nullptr;
            }

            _superOwned = std::make_shared<SerializedStruct>(Context, *superStruct);
            return _superOwned.get();
        }
        return nullptr;
    }
}
