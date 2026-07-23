// Ported from CUE4Parse/UE4/IO/Objects/FExportBundleEntry.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::IO::Objects
{
    enum class EExportCommandType : uint32_t
    {
        ExportCommandType_Create,
        ExportCommandType_Serialize,
        ExportCommandType_Count
    };

    struct FExportBundleEntry
    {
        uint32_t LocalExportIndex = 0;
        EExportCommandType CommandType = EExportCommandType::ExportCommandType_Create;
    };
    static_assert(sizeof(FExportBundleEntry) == 8);
}
