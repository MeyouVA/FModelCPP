// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseMediaCookedData.cs
// One cooked media (.wem) entry: an id plus where its bytes are packaged.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "FWwisePackagedFile.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;
    struct FWwiseMediaCookedData
    {
        uint32_t MediaId = 0;
        FName MediaPathName;
        int32_t PrefetchSize = 0;
        int32_t MemoryAlignment = 0;
        bool bDeviceMemory = false;
        bool bStreaming = false;
        FName DebugName;
        std::shared_ptr<FWwisePackagedFile> PackagedFile;

        FWwiseMediaCookedData() = default;

        explicit FWwiseMediaCookedData(const FStructFallback& fallback)
        {
            // C# reads the id as int then casts: the property is signed on the wire.
            MediaId = static_cast<uint32_t>(PropertyUtil::GetOrDefault<int32_t>(fallback, "MediaId"));
            MediaPathName = PropertyUtil::GetOrDefault<FName>(fallback, "MediaPathName");
            PrefetchSize = PropertyUtil::GetOrDefault<int32_t>(fallback, "PrefetchSize");
            MemoryAlignment = PropertyUtil::GetOrDefault<int32_t>(fallback, "MemoryAlignment");
            bDeviceMemory = PropertyUtil::GetOrDefault<bool>(fallback, "bDeviceMemory");
            bStreaming = PropertyUtil::GetOrDefault<bool>(fallback, "bStreaming");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
            PackagedFile = FWwisePackagedFile::CreatePackagedFile(fallback, "PackagedFile");
        }

        void SerializeBulkData(FAssetArchive& Ar) const
        {
            if (PackagedFile != nullptr) PackagedFile->SerializeBulkData(Ar);
        }
    };
}
