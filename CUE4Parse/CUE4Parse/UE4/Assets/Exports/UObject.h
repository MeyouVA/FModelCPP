// Ported from CUE4Parse/UE4/Assets/Exports/UObject.cs (core object + tagged-property deserialization).
//
// Deliberate differences from C#:
//   * Both property paths are ported: tagged (versioned) and unversioned (FUnversionedHeader + mappings via
//     Ar.Owner->Mappings()). DeserializeRawProperties (the blueprint-decompiler raw path) is still deferred. TODO.
//   * The reflection-based property accessors (GetOrDefault<T>/Get<T> via PropertyUtil) are omitted; consumers
//     read the Properties list directly and dynamic_cast a tag's value. The JSON WriteJson path is omitted.
//   * SparseClassData (UE5 BlueprintGeneratedClass) handling and GetFullName/GetPathName (which need object
//     loading) are deferred. TODO.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "EObjectFlags.h"
#include "../Objects/FPropertyTag.h"
#include "../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Assets { class ResolvedObject; class IPackage; }
namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }
namespace CUE4Parse::UE4::Objects::UObject { class UStruct; }

namespace CUE4Parse::UE4::Assets::Exports
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using Objects::FPropertyTag;

    class UObject
    {
    public:
        std::string Name;
        IPackage* Owner = nullptr; // The package this export was loaded from (C#'s UObject.Owner). Null until loaded.
        ResolvedObject* Class = nullptr;
        ResolvedObject* Outer = nullptr;
        ResolvedObject* Super = nullptr;
        ResolvedObject* Template = nullptr;
        std::optional<FGuid> ObjectGuid;
        EObjectFlags Flags = RF_NoFlags;
        std::vector<FPropertyTag> Properties;

        UObject() = default;
        virtual ~UObject() = default;

        // Reads the object's tagged properties (up to validPos) plus the trailing optional ObjectGuid.
        virtual void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos);

        virtual void PostLoad() {}

        std::string ToString() const { return Name; }

        // Reads tagged properties until the terminating "None" tag.
        static void DeserializePropertiesTagged(std::vector<FPropertyTag>& properties, Readers::FAssetArchive& Ar, bool isStruct);

        // Reads unversioned properties: an FUnversionedHeader, then each flagged value through the struct's
        // mappings schema (Ar.Owner->Mappings() for a UScriptClass, a SerializedStruct otherwise).
        static void DeserializePropertiesUnversioned(std::vector<FPropertyTag>& properties, Readers::FAssetArchive& Ar,
                                                     const CUE4Parse::UE4::Objects::UObject::UStruct& struc);
    };
}
