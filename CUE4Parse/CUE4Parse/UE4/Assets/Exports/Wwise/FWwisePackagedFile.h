// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwisePackagedFile.cs
// Where one cooked Wwise file (a bank or a media blob) actually lives. The PackagingStrategy decides:
// BulkData means the bytes follow inline as an FByteBulkData and get parsed straight into a WwiseReader;
// External/AdditionalFile means they live somewhere else entirely and this is only a reference.
//
// Deliberate differences from C#:
//   * No logging: C# logs the three failure/unsupported paths in SerializeBulkData. The control flow is
//     kept exactly, including the RIFFSectionSizeException retry through TryCombineBulkData.
//   * BulkData is a unique_ptr (C# holds a nullable reference).
#pragma once

#include <memory>
#include <string>

#include "../PropertyUtil.h"
#include "../../Objects/FByteBulkData.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Wwise/WwiseArchive.h"
#include "../../../Wwise/WwiseReader.h"
#include "EWwisePackagingStrategy.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Wwise::FWwiseArchive;
    using CUE4Parse::UE4::Wwise::RIFFSectionSizeException;
    using CUE4Parse::UE4::Wwise::WwiseDataSource;
    using CUE4Parse::UE4::Wwise::WwiseReader;

    class FWwisePackagedFile : public FStructFallback
    {
    public:
        EWwisePackagingStrategy PackagingStrategy = EWwisePackagingStrategy::Source;
        FName PathName;
        FName ModularGameplayName;
        bool bStreaming = false;
        int32_t PrefetchSize = 0;
        int32_t MemoryAlignment = 0;
        bool bDeviceMemory = false;
        uint32_t Hash = 0;
        std::unique_ptr<WwiseReader> BulkData;

        // C# has two constructors that read the same eight fields, one off an existing fallback and one
        // that reads a fresh "WwisePackagedFile" struct from the archive. Both are kept.
        explicit FWwisePackagedFile(const FStructFallback& fallback) { ReadFields(fallback); }

        explicit FWwisePackagedFile(FAssetArchive& Ar) : FStructFallback(Ar, std::string("WwisePackagedFile"))
        {
            ReadFields(*this);
        }

        // C#'s static CreatePackagedFile: null when the named property is not a struct.
        static std::unique_ptr<FWwisePackagedFile> CreatePackagedFile(const FStructFallback& fallback,
                                                                     const std::string& propertyName)
        {
            const FStructFallback* pfFallback = nullptr;
            if (!PropertyUtil::TryGet<const FStructFallback*>(fallback, propertyName, pfFallback) || pfFallback == nullptr)
                return nullptr;
            return std::make_unique<FWwisePackagedFile>(*pfFallback);
        }

        void SerializeBulkData(FAssetArchive& Ar)
        {
            if (PackagingStrategy == EWwisePackagingStrategy::BulkData)
            {
                FByteBulkData bulkData(Ar);
                auto dataAr = bulkData.TryCreateReader("AkAssetData");
                if (dataAr == nullptr) return;

                try
                {
                    FWwiseArchive reader(*dataAr);
                    BulkData = std::make_unique<WwiseReader>(reader, WwiseDataSource::FromBulkData(Ar, bulkData));
                }
                // i know it's ugly, but i don't see other solution without rewriting everything
                catch (const RIFFSectionSizeException&)
                {
                    std::vector<uint8_t> combinedData;
                    std::unique_ptr<FByteBulkData> fullBulkData;
                    if (bulkData.TryCombineBulkData(Ar, combinedData, fullBulkData))
                    {
                        try
                        {
                            FWwiseArchive reader("AkAssetData", std::move(combinedData), Ar.Versions);
                            if (fullBulkData != nullptr)
                                BulkData = std::make_unique<WwiseReader>(
                                    reader, WwiseDataSource::FromBulkData(Ar, *fullBulkData));
                            else
                                BulkData = std::make_unique<WwiseReader>(reader, WwiseDataSource::Archive());
                        }
                        catch (...)
                        {
                            // C# logs "Failed to read Wwise bank data ... from combined bulk data".
                        }
                    }
                }
                catch (...)
                {
                    // C# logs "Failed to read Wwise bank data ... from bulk data".
                }
            }
            else if (PackagingStrategy == EWwisePackagingStrategy::External ||
                     PackagingStrategy == EWwisePackagingStrategy::AdditionalFile)
            {
                // maybe in AssetLibrary or an asset
            }
            else
            {
                // C# warns that the packaging strategy is unsupported.
            }
        }

    private:
        void ReadFields(const FStructFallback& source)
        {
            PackagingStrategy = PropertyUtil::GetOrDefault<EWwisePackagingStrategy>(
                source, "PackagingStrategy", EWwisePackagingStrategy::Source);
            PathName = PropertyUtil::GetOrDefault<FName>(source, "PathName");
            ModularGameplayName = PropertyUtil::GetOrDefault<FName>(source, "ModularGameplayName");
            bStreaming = PropertyUtil::GetOrDefault<bool>(source, "bStreaming");
            PrefetchSize = PropertyUtil::GetOrDefault<int32_t>(source, "PrefetchSize");
            MemoryAlignment = PropertyUtil::GetOrDefault<int32_t>(source, "MemoryAlignment");
            bDeviceMemory = PropertyUtil::GetOrDefault<bool>(source, "bDeviceMemory");
            Hash = PropertyUtil::GetOrDefault<uint32_t>(source, "Hash");
        }
    };
}
