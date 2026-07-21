// Ported from CUE4Parse/UE4/Objects/UObject/UnrealType.cs
// The FProperty hierarchy (FField subclasses) making up a UStruct's ChildProperties in the FProperties system.
//
// Deliberate differences from C#:
//   * EPropertyFlags is kept as an opaque 64-bit value (only `None` named): the ported readers never branch on
//     individual flags (that only happens in the un-ported BlueprintDecompiler / JSON layers). The full named
//     set can be filled in when those arrive. TODO.
//   * FProperty::GetAccessMode (decompiler-only) and every WriteJson override are omitted.
//   * The Verse* and Reference/Utf8Str property types are ported (they're simple); the Borderlands4
//     GameDataHandle/GbxDefPtr types are not (see FField::Construct). TODO.
#pragma once

#include <cstdint>
#include <memory>

#include "FField.h"
#include "CoreNetTypes.h"
#include "ObjectResource.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    // Opaque property flags (see note above).
    enum class EPropertyFlags : uint64_t { None = 0 };

    class FProperty : public FField
    {
    public:
        int32_t ArrayDim = 0;
        int32_t ElementSize = 0;
        EPropertyFlags PropertyFlags = EPropertyFlags::None;
        uint16_t RepIndex = 0;
        FName RepNotifyFunc;
        ELifetimeCondition BlueprintReplicationCondition = ELifetimeCondition::COND_None;

        void Deserialize(FAssetArchive& Ar) override;
    };

    class FNumericProperty : public FProperty {};

    class FArrayProperty : public FProperty
    {
    public:
        std::unique_ptr<FProperty> Inner;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FBoolProperty : public FProperty
    {
    public:
        uint8_t FieldSize = 0;
        uint8_t ByteOffset = 0;
        uint8_t ByteMask = 0;
        uint8_t FieldMask = 0;
        uint8_t BoolSize = 0;
        bool bIsNativeBool = false;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FByteProperty : public FNumericProperty
    {
    public:
        FPackageIndex Enum;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FObjectProperty : public FProperty
    {
    public:
        FPackageIndex PropertyClass;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FClassProperty : public FObjectProperty
    {
    public:
        FPackageIndex MetaClass;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FDelegateProperty : public FProperty
    {
    public:
        FPackageIndex SignatureFunction;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FEnumProperty : public FProperty
    {
    public:
        std::unique_ptr<FNumericProperty> UnderlyingProp;
        FPackageIndex Enum;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FFieldPathProperty : public FProperty
    {
    public:
        FName PropertyClass;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FDoubleProperty : public FNumericProperty {};
    class FFloatProperty : public FNumericProperty {};
    class FInt16Property : public FNumericProperty {};
    class FInt64Property : public FNumericProperty {};
    class FInt8Property : public FNumericProperty {};
    class FIntProperty : public FNumericProperty {};

    class FInterfaceProperty : public FProperty
    {
    public:
        FPackageIndex InterfaceClass;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FMapProperty : public FProperty
    {
    public:
        std::unique_ptr<FProperty> KeyProp;
        std::unique_ptr<FProperty> ValueProp;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FMulticastDelegateProperty : public FProperty
    {
    public:
        FPackageIndex SignatureFunction;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FMulticastInlineDelegateProperty : public FProperty
    {
    public:
        FPackageIndex SignatureFunction;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FNameProperty : public FProperty {};

    class FSoftClassProperty : public FClassProperty {};
    class FSoftObjectProperty : public FObjectProperty {};

    class FSetProperty : public FProperty
    {
    public:
        std::unique_ptr<FProperty> ElementProp;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FStrProperty : public FProperty {};
    class FUtf8StrProperty : public FProperty {};

    class FStructProperty : public FProperty
    {
    public:
        FPackageIndex Struct;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FTextProperty : public FProperty {};

    class FUInt16Property : public FNumericProperty {};
    class FUInt32Property : public FNumericProperty {};
    class FUInt64Property : public FNumericProperty {};

    class FWeakObjectProperty : public FObjectProperty {};

    class FOptionalProperty : public FProperty
    {
    public:
        std::unique_ptr<FProperty> ValueProperty;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FVerseStringProperty : public FProperty
    {
    public:
        std::unique_ptr<FProperty> ValueProperty;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FVerseFunctionProperty : public FProperty
    {
    public:
        FPackageIndex Function;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FVerseClassProperty : public FClassProperty
    {
    public:
        bool bRequiresConcrete = false;
        bool bRequiresCastable = false;
        void Deserialize(FAssetArchive& Ar) override;
    };

    class FVerseDynamicProperty : public FProperty {};
    class FReferenceProperty : public FProperty {};
}
