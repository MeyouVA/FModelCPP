// Ported from CUE4Parse/UE4/IO/Objects/FZenPackageSummary.cs
#include "FZenPackageSummary.h"

namespace CUE4Parse::UE4::IO::Objects
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::UE4::Objects::Core::Serialization::ECustomVersionSerializationFormat;

    FZenPackageVersioningInfo::FZenPackageVersioningInfo(Readers::FArchive& Ar)
    {
        ZenVersion = static_cast<EZenPackageVersion>(Ar.Read<uint32_t>());
        PackageVersion = FPackageFileVersion(Ar.Read<int32_t>(), Ar.Read<int32_t>());
        LicenseeVersion = Ar.Read<int32_t>();
        CustomVersions = FCustomVersionContainer(Ar, ECustomVersionSerializationFormat::Latest);
    }

    FZenPackageSummary::FZenPackageSummary(Readers::FArchive& Ar)
    {
        bHasVersioningInfo = Ar.Read<uint32_t>();
        HeaderSize = Ar.Read<uint32_t>();
        Name = Ar.Read<FMappedName>();
        PackageFlags = static_cast<CUE4Parse::UE4::Objects::UObject::EPackageFlags>(Ar.Read<uint32_t>());
        CookedHeaderSize = Ar.Read<uint32_t>();
        ImportedPublicExportHashesOffset = Ar.Read<int32_t>();
        ImportMapOffset = Ar.Read<int32_t>();
        ExportMapOffset = Ar.Read<int32_t>();
        ExportBundleEntriesOffset = Ar.Read<int32_t>();

        if (Ar.Game() >= GAME_UE5_3)
        {
            DependencyBundleHeadersOffset = Ar.Read<int32_t>();
            DependencyBundleEntriesOffset = Ar.Read<int32_t>();
            ImportedPackageNamesOffset = Ar.Read<int32_t>();
        }
        else
        {
            GraphDataOffset = Ar.Read<int32_t>();
        }
    }
}
