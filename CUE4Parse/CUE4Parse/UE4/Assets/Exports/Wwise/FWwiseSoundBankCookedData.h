// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseSoundBankCookedData.cs
// One cooked soundbank entry. FWwiseInitBankCookedData extends it, so SerializeBulkData is virtual.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "EWwiseSoundBankType.h"
#include "FWwisePackagedFile.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FWwiseSoundBankCookedData
    {
    public:
        uint32_t SoundBankId = 0;
        FName SoundBankPathName;
        int32_t MemoryAlignment = 0;
        bool bDeviceMemory = false;
        bool bContainsMedia = false;
        EWwiseSoundBankType SoundBankType = static_cast<EWwiseSoundBankType>(0);
        FName DebugName;
        std::shared_ptr<FWwisePackagedFile> PackagedFile;

        FWwiseSoundBankCookedData() = default;
        virtual ~FWwiseSoundBankCookedData() = default;

        explicit FWwiseSoundBankCookedData(const FStructFallback& fallback)
        {
            SoundBankId = static_cast<uint32_t>(PropertyUtil::GetOrDefault<int32_t>(fallback, "SoundBankId"));
            SoundBankPathName = PropertyUtil::GetOrDefault<FName>(fallback, "SoundBankPathName");
            MemoryAlignment = PropertyUtil::GetOrDefault<int32_t>(fallback, "MemoryAlignment");
            bDeviceMemory = PropertyUtil::GetOrDefault<bool>(fallback, "bDeviceMemory");
            bContainsMedia = PropertyUtil::GetOrDefault<bool>(fallback, "bContainsMedia");
            SoundBankType = PropertyUtil::GetOrDefault<EWwiseSoundBankType>(fallback, "SoundBankType");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
            PackagedFile = FWwisePackagedFile::CreatePackagedFile(fallback, "PackagedFile");
        }

        virtual void SerializeBulkData(FAssetArchive& Ar) const
        {
            if (PackagedFile != nullptr) PackagedFile->SerializeBulkData(Ar);
        }
    };
}
