// Ported from CUE4Parse/UE4/IO/Objects/FExportMapEntry.cs
// One Zen export-map record: cooked serial range, mapped object name, the object-index links and (UE5+)
// the public export hash. Always 72 bytes on disk (the ctor re-seeks to start + Size).
#pragma once

#include <cstdint>

#include "FMappedName.h"
#include "FPackageObjectIndex.h"
#include "../../Assets/Exports/EObjectFlags.h"
#include "../../Readers/FArchive.h"
#include "../../Versions/EGame.h"

namespace CUE4Parse::UE4::IO::Objects
{
    struct FExportMapEntry
    {
        static constexpr int Size = 72;

        uint64_t CookedSerialOffset = 0;
        uint64_t CookedSerialSize = 0;
        FMappedName ObjectName;
        FPackageObjectIndex OuterIndex;
        FPackageObjectIndex ClassIndex;
        FPackageObjectIndex SuperIndex;
        FPackageObjectIndex TemplateIndex;
        FPackageObjectIndex GlobalImportIndex;
        uint64_t PublicExportHash = 0;
        CUE4Parse::UE4::Assets::Exports::EObjectFlags ObjectFlags = CUE4Parse::UE4::Assets::Exports::RF_NoFlags;
        uint8_t FilterFlags = 0; // EExportFilterFlags: client/server flags

        FExportMapEntry() = default;

        explicit FExportMapEntry(Readers::FArchive& Ar)
        {
            const int64_t start = Ar.Position;
            CookedSerialOffset = Ar.Read<uint64_t>();
            CookedSerialSize = Ar.Read<uint64_t>();
            ObjectName = Ar.Read<FMappedName>();
            OuterIndex = Ar.Read<FPackageObjectIndex>();
            ClassIndex = Ar.Read<FPackageObjectIndex>();
            SuperIndex = Ar.Read<FPackageObjectIndex>();
            TemplateIndex = Ar.Read<FPackageObjectIndex>();
            if (Ar.Game() >= Versions::GAME_UE5_0)
            {
                GlobalImportIndex = FPackageObjectIndex::InvalidObjectIndex();
                PublicExportHash = Ar.Read<uint64_t>();
            }
            else
            {
                GlobalImportIndex = Ar.Read<FPackageObjectIndex>();
                PublicExportHash = 0;
            }

            ObjectFlags = static_cast<CUE4Parse::UE4::Assets::Exports::EObjectFlags>(Ar.Read<uint32_t>());
            FilterFlags = Ar.Read<uint8_t>();
            Ar.Position = start + Size;
        }
    };
}
