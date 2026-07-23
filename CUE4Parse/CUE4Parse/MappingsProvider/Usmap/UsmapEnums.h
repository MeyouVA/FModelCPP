// Ported from CUE4Parse/MappingsProvider/Usmap/EPropertyType.cs, EUsmapCompressionMethod.cs and
// EUsmapVersion.cs (three tiny enum files folded into one header).
#pragma once

#include <cstdint>

namespace CUE4Parse::MappingsProvider::Usmap
{
    enum class EPropertyType : uint8_t
    {
        ByteProperty,
        BoolProperty,
        IntProperty,
        FloatProperty,
        ObjectProperty,
        NameProperty,
        DelegateProperty,
        DoubleProperty,
        ArrayProperty,
        StructProperty,
        StrProperty,
        TextProperty,
        InterfaceProperty,
        MulticastDelegateProperty,
        WeakObjectProperty,   //
        LazyObjectProperty,   // When deserialized, these 3 properties will be SoftObjects
        AssetObjectProperty,  //
        SoftObjectProperty,
        UInt64Property,
        UInt32Property,
        UInt16Property,
        Int64Property,
        Int16Property,
        Int8Property,
        MapProperty,
        SetProperty,
        EnumProperty,
        FieldPathProperty,
        OptionalProperty,
        Utf8StrProperty,
        AnsiStrProperty,

        ClassProperty,
        MulticastInlineDelegateProperty,
        SoftClassProperty,
        VerseStringProperty,
        VerseDynamicProperty,
        VerseFunctionProperty,

        CustomProperty_FD = 0xFD,
        CustomProperty_FE = 0xFE,
        Unknown = 0xFF
    };

    // C#'s Enum.GetName(typeEnum) for the values above; empty string for an unnamed value (C# null -> "").
    const char* PropertyTypeName(EPropertyType type);

    enum class EUsmapCompressionMethod : uint8_t
    {
        None,
        Oodle,
        Brotli,
        ZStandard,

        Unknown = 0xFF
    };

    enum class EUsmapVersion : uint8_t
    {
        /* Initial format. */
        Initial,

        /* Adds package versioning to aid with compatibility */
        PackageVersioning,

        /* Adds support for 16-bit wide name-lengths (ushort/uint16) */
        LongFName,

        /* Adds support for enums with more than 255 values */
        LargeEnums,

        /* Adds support for explicit enum values */
        ExplicitEnumValues,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };
}
