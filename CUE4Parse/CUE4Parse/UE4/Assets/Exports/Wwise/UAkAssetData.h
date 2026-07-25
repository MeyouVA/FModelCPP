// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAssetData.cs
// Bank bytes attached to an export as bulk data. Everything after the properties is one FByteBulkData that
// parses straight into a WwiseReader.
#pragma once

#include <memory>

#include "../../Objects/FByteBulkData.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Wwise/WwiseArchive.h"
#include "../../../Wwise/WwiseReader.h"
#include "../UObject.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Wwise::FWwiseArchive;
    using CUE4Parse::UE4::Wwise::WwiseDataSource;
    using CUE4Parse::UE4::Wwise::WwiseReader;

    class UAkAssetData : public UObject
    {
    public:
        std::unique_ptr<WwiseReader> Data;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;

            // The bulk data must outlive the reader: the reader records deferred ranges into it rather
            // than copying the bytes out. C# leans on the GC for the same lifetime.
            _bulkData = std::make_unique<FByteBulkData>(Ar);
            auto dataAr = _bulkData->TryCreateReader("AkAssetData");
            if (dataAr == nullptr) return;

            _dataAr = std::move(dataAr);
            FWwiseArchive reader(*_dataAr);
            Data = std::make_unique<WwiseReader>(reader, WwiseDataSource::FromBulkData(Ar, *_bulkData));
        }

    private:
        std::unique_ptr<FByteBulkData> _bulkData;
        std::unique_ptr<Readers::FArchive> _dataAr;
    };
}
