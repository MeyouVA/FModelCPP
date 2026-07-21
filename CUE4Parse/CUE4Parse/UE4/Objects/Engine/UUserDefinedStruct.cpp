// Ported from CUE4Parse/UE4/Objects/Engine/UUserDefinedStruct.cs.
#include "UUserDefinedStruct.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Assets/Objects/Properties/EnumProperty.h"
#include "../../Assets/Objects/Properties/ByteProperty.h"

namespace CUE4Parse::UE4::Objects::Engine
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Assets::Objects::Properties::EnumProperty;
    using CUE4Parse::UE4::Assets::Objects::Properties::ByteProperty;

    namespace
    {
        // C# does GetOrDefault<EUserDefinedStructureStatus>("Status"); this port has no reflection accessor, so
        // scan the tagged Properties list. The value is an EnumProperty holding an FName like
        // "EUserDefinedStructureStatus::UDSS_Dirty" (or, on older assets, a raw ByteProperty). Absent -> default.
        EUserDefinedStructureStatus ReadStatus(const std::vector<CUE4Parse::UE4::Assets::Objects::FPropertyTag>& properties)
        {
            for (const auto& tag : properties)
            {
                if (tag.Name.Text() != "Status")
                    continue;

                if (const auto* ep = dynamic_cast<const EnumProperty*>(tag.Tag.get()))
                {
                    const std::string full = ep->Value.Text();
                    const size_t sep = full.rfind("::");
                    const std::string member = sep == std::string::npos ? full : full.substr(sep + 2);
                    if (member == "UDSS_Dirty") return EUserDefinedStructureStatus::UDSS_Dirty;
                    if (member == "UDSS_Error") return EUserDefinedStructureStatus::UDSS_Error;
                    if (member == "UDSS_Duplicate") return EUserDefinedStructureStatus::UDSS_Duplicate;
                    return EUserDefinedStructureStatus::UDSS_UpToDate;
                }
                if (const auto* bp = dynamic_cast<const ByteProperty*>(tag.Tag.get()))
                {
                    return static_cast<EUserDefinedStructureStatus>(bp->Value);
                }
                break;
            }
            return EUserDefinedStructureStatus::UDSS_UpToDate; // GetOrDefault default (enum 0)
        }
    }

    void UUserDefinedStruct::Deserialize(FAssetArchive& Ar, int64_t validPos)
    {
        UStruct::Deserialize(Ar, validPos);

        Status = ReadStatus(Properties);

        if (Flags & CUE4Parse::UE4::Assets::Exports::RF_ClassDefaultObject) return;
        if (Status != EUserDefinedStructureStatus::UDSS_UpToDate) return;

        StructFlags = Ar.Read<uint32_t>();

        // assume-modern: FFrameworkObjectVersion >= UserDefinedStructsStoreDefaultInstance, so the default
        // instance (a tagged property list, self-terminated by a None tag) follows. Guarded so we never over-read
        // on an asset that predates the block. TODO: gate on the real custom version.
        if (Ar.Position < validPos)
            DeserializePropertiesTagged(DefaultProperties, Ar, true);
    }
}
