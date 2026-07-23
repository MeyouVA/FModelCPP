// Ported from CUE4Parse/UE4/IO/Objects/FIoContainerHeader.cs
#include "FIoContainerHeader.h"

#include "../../Exceptions/ParserException.h"
#include "../../Versions/EGame.h"

namespace CUE4Parse::UE4::IO::Objects
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::UE4::Exceptions::ParserException;

    FIoContainerHeaderSoftPackageReferences::FIoContainerHeaderSoftPackageReferences(Readers::FArchive& Ar)
    {
        bContainsSoftPackageReferences = Ar.ReadBoolean();
        if (bContainsSoftPackageReferences)
        {
            PackageIds = Ar.ReadArrayCounted<FPackageId>();
            PackageIndices = Ar.ReadArrayCounted<uint8_t>();
        }
    }

    FIoContainerHeader::FIoContainerHeader(Readers::FArchive& Ar)
    {
        Version = Ar.Game() >= GAME_UE5_0 ? EIoContainerHeaderVersion::Initial
                                          : EIoContainerHeaderVersion::BeforeVersionWasAdded;
        if (Version == EIoContainerHeaderVersion::Initial)
        {
            const auto signature = Ar.Read<uint32_t>();
            if (signature != static_cast<uint32_t>(Signature))
            {
                char msg[96];
                std::snprintf(msg, sizeof(msg), "Invalid container header signature: 0x%08X != 0x%08X",
                              signature, static_cast<uint32_t>(Signature));
                throw ParserException(Ar, msg);
            }

            Version = static_cast<EIoContainerHeaderVersion>(Ar.Read<int32_t>());
        }

        ContainerId = Ar.Read<FIoContainerId>();
        if (Version < EIoContainerHeaderVersion::OptionalSegmentPackages)
            Ar.Read<uint32_t>(); // packageCount (unused)
        if (Version == EIoContainerHeaderVersion::BeforeVersionWasAdded)
        {
            // Kept verbatim from C#: nameHashesSize is read immediately (the names blob is NOT skipped
            // first), and the name batch is then re-read from namesPos.
            const auto namesSize = Ar.Read<int32_t>();
            const int64_t namesPos = Ar.Position;
            const auto nameHashesSize = Ar.Read<int32_t>();
            const int64_t continuePos = Ar.Position + nameHashesSize;
            if (namesSize > 0 && nameHashesSize > 0)
            {
                Ar.Position = namesPos;
                ContainerNameMap = FNameEntrySerialized::LoadNameBatch(
                    Ar, static_cast<int>(nameHashesSize / sizeof(uint64_t)) - 1);
            }
            Ar.Position = continuePos;
        }

        ReadPackageIdsAndEntries(Ar, PackageIds, StoreEntries);

        if (Version >= EIoContainerHeaderVersion::OptionalSegmentPackages)
        {
            ReadPackageIdsAndEntries(Ar, OptionalSegmentPackageIds, OptionalSegmentStoreEntries);
        }
        if (Version >= EIoContainerHeaderVersion::Initial)
        {
            ContainerNameMap = FNameEntrySerialized::LoadNameBatch(Ar);
        }
        if (Version >= EIoContainerHeaderVersion::LocalizedPackages)
        {
            LocalizedPackages = Ar.ReadArrayCounted<FIoContainerHeaderLocalizedPackage>();
        }
        PackageRedirects = Ar.ReadArrayCounted<FIoContainerHeaderPackageRedirect>();
        if (Version == EIoContainerHeaderVersion::SoftPackageReferences)
        {
            SoftPackageReferences = FIoContainerHeaderSoftPackageReferences(Ar);
        }
        else if (Version >= EIoContainerHeaderVersion::SoftPackageReferencesOffset)
        {
            SoftPackageReferencesSerialInfo = FIoContainerHeaderSerialInfo(Ar);
        }
    }

    void FIoContainerHeader::ReadPackageIdsAndEntries(Readers::FArchive& Ar, std::vector<FPackageId>& packageIds,
                                                      std::vector<FFilePackageStoreEntry>& storeEntries)
    {
        packageIds = Ar.ReadArrayCounted<FPackageId>();
        const auto storeEntriesSize = Ar.Read<int32_t>();
        const int64_t storeEntriesEnd = Ar.Position + storeEntriesSize;
        storeEntries.clear();
        storeEntries.reserve(packageIds.size());
        for (size_t i = 0; i < packageIds.size(); i++)
            storeEntries.emplace_back(Ar, Version);
        Ar.Position = storeEntriesEnd;
    }
}
