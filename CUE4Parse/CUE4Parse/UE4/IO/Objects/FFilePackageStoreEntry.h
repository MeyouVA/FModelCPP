// Ported from CUE4Parse/UE4/IO/Objects/FFilePackageStoreEntry.cs
// One container-header store entry: a package's export counts (older versions) and its imported package
// ids (read via TCArrayView: count + offset-to-data-from-view-start).
//
// EIoContainerHeaderVersion lives here (not in FIoContainerHeader.h) because the container header includes
// this file; in C# both are namespace-level types in separate files.
//
// Deliberate difference from C#: the commented-out ShaderMapHashes stay unread (C# skips 8 bytes for the
// view), matching the C# behaviour.
#pragma once

#include <cstdint>
#include <vector>

#include "FPackageId.h"
#include "../../Readers/FArchive.h"
#include "../../Versions/EGame.h"

namespace CUE4Parse::UE4::IO::Objects
{
    // From FIoContainerHeader.cs.
    enum class EIoContainerHeaderVersion : int32_t
    {
        BeforeVersionWasAdded = -1, // Custom constant to indicate pre-UE5 data
        Initial = 0,
        LocalizedPackages = 1,
        OptionalSegmentPackages = 2,
        NoExportInfo = 3,
        SoftPackageReferences = 4,
        SoftPackageReferencesOffset = 5,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    class FFilePackageStoreEntry
    {
    public:
        int32_t ExportCount = 0;
        int32_t ExportBundleCount = 0;
        std::vector<FPackageId> ImportedPackages;
        //std::vector<FSHAHash> ShaderMapHashes;

        FFilePackageStoreEntry() = default;

        FFilePackageStoreEntry(Readers::FArchive& Ar, EIoContainerHeaderVersion version)
        {
            if (version >= EIoContainerHeaderVersion::Initial)
            {
                if (version < EIoContainerHeaderVersion::NoExportInfo)
                {
                    ExportCount = Ar.Read<int32_t>();
                    ExportBundleCount = Ar.Read<int32_t>();
                }

                ImportedPackages = ReadCArrayView<FPackageId>(Ar);
                Ar.Position += 8; // ShaderMapHashes view (unread)
            }
            else if (Ar.Game() == Versions::GAME_eFootball)
            {
                ExportCount = Ar.Read<int32_t>();
                ExportBundleCount = Ar.Read<int32_t>();
                ImportedPackages = ReadCArrayView<FPackageId>(Ar);
            }
            else
            {
                Ar.Position += 8; // ExportBundlesSize
                ExportCount = Ar.Read<int32_t>();
                ExportBundleCount = Ar.Read<int32_t>();
                Ar.Position += 8; // LoadOrder + Pad
                ImportedPackages = ReadCArrayView<FPackageId>(Ar);
                if (Ar.Game() == Versions::GAME_NeedForSpeedMobile) Ar.Position += 8;
            }
        }

    private:
        template <typename T>
        static std::vector<T> ReadCArrayView(Readers::FArchive& Ar)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            const int64_t initialPos = Ar.Position;
            const int32_t arrayNum = Ar.Read<int32_t>();
            const int32_t offsetToDataFromThis = Ar.Read<int32_t>();
            if (arrayNum == 0)
                return {};

            const int64_t continuePos = Ar.Position;
            Ar.Position = initialPos + offsetToDataFromThis;
            auto result = Ar.ReadArray<T>(arrayNum);
            Ar.Position = continuePos;
            return result;
        }
    };
}
