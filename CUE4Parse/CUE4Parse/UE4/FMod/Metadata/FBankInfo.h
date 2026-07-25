// Ported from CUE4Parse/UE4/FMod/Metadata/FBankInfo.cs
// The BNKI chunk: the bank's own GUID and versioned metadata. Held by unique_ptr on FModReader (C#'s
// nullable struct field).
#pragma once

#include "../Objects/FModGuid.h"
#include "../Enums/EFModVersion.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Metadata
{
    struct FBankInfo
    {
        Objects::FModGuid BaseGuid;
        uint64_t Hash = 0;
        Enums::EFModVersion FileVersion{};
        int32_t TopLevelEventCount = 0;
        int32_t ExportFlags = 0;

        FBankInfo() = default;
        explicit FBankInfo(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            if (FModReader::Version() >= 0x37) Hash = Ar.Read<uint64_t>();
            FileVersion = static_cast<Enums::EFModVersion>(FModReader::Version());
            if (FModReader::Version() >= 0x41) TopLevelEventCount = Ar.Read<int32_t>();
            if (FModReader::Version() >= 0x4D) ExportFlags = Ar.Read<int32_t>();
        }
    };
}
