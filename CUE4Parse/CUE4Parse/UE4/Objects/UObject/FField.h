// Ported from CUE4Parse/UE4/Objects/UObject/FField.cs
// FField is the base of the FProperties reflection system (UE4.25+): the serialized property descriptors that
// live on a UStruct's ChildProperties. FProperty and its subclasses are in UnrealType.{h,cpp}.
//
// Deliberate differences from C#:
//   * FField::Construct throws (ParserException) on the Borderlands4 game-specific field types
//     (GameDataHandleProperty / GbxDefPtrProperty) — those readers aren't ported. TODO.
//   * The WriteJson path and the FFieldConverter are omitted.
#pragma once

#include <memory>
#include <string>

#include "FName.h"
#include "../../Assets/Exports/EObjectFlags.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    using CUE4Parse::UE4::Assets::Exports::EObjectFlags;
    using Assets::Readers::FAssetArchive;

    class FField
    {
    public:
        FName Name;
        EObjectFlags Flags = CUE4Parse::UE4::Assets::Exports::RF_NoFlags;

        FField() = default;
        virtual ~FField() = default;

        virtual void Deserialize(FAssetArchive& Ar);

        std::string ToString() const { return Name.Text(); }

        // Factory keyed on the serialized field type name (e.g. "IntProperty"). Definitions live in
        // UnrealType.cpp (they need the concrete FProperty subclasses).
        static std::unique_ptr<FField> Construct(const FName& fieldTypeName);
        // Reads a field type name then, if not "None", constructs + deserializes it (else nullptr).
        static std::unique_ptr<FField> SerializeSingleField(FAssetArchive& Ar);
    };
}
