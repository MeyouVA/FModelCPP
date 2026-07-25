// Ported from CUE4Parse/UE4/Assets/Objects/FDeferredByteData.cs
// (C# puts the file under Assets/Objects but declares namespace CUE4Parse.UE4.Wwise; the port follows the
// *namespace*, as it does elsewhere when the two disagree.)
//
// "Some bytes I can fetch later." A Wwise bank names hundreds of embedded .wem payloads; materialising all
// of them while parsing would read the whole archive, so each one is recorded as one of these instead and
// only read when someone asks for it. The three subclasses are the three places the bytes can come from:
// already in memory, somewhere in a mounted game file, or inside an export's bulk data.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../Assets/Objects/FByteBulkData.h"
#include "../Assets/Objects/FByteBulkDataHeader.h"
#include "../Assets/Readers/FAssetArchive.h"
#include "../../FileProvider/Objects/GameFile.h"

namespace CUE4Parse::UE4::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::EBulkDataFlags;
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Objects::FByteBulkDataHeader;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::IO::Objects::FBulkDataCookedIndex;
    using CUE4Parse::FileProvider::Objects::GameFile;

    class FDeferredByteData
    {
    public:
        virtual ~FDeferredByteData() = default;

        virtual bool IsValid() const = 0;
        virtual std::vector<uint8_t> GetData() const = 0;
        virtual int64_t LoadedSize() const { return 0; }
    };

    // The bytes are already here.
    class FArrayDeferredByteData final : public FDeferredByteData
    {
    public:
        explicit FArrayDeferredByteData(std::vector<uint8_t> data) : _data(std::move(data)) {}

        bool IsValid() const override { return !_data.empty(); }
        std::vector<uint8_t> GetData() const override { return _data; }
        int64_t LoadedSize() const override { return static_cast<int64_t>(_data.size()); }

    private:
        std::vector<uint8_t> _data;
    };

    // A range of a mounted game file. With offset 0 / size -1 the whole file is meant and no header is
    // built, which is what makes the "loose .wem on disk" case a plain full read.
    class FGameFileDeferredByteData final : public FDeferredByteData
    {
    public:
        std::shared_ptr<GameFile> File;

        explicit FGameFileDeferredByteData(std::shared_ptr<GameFile> file, int64_t offset = 0, int32_t size = -1)
            : File(std::move(file))
        {
            if (!(offset == 0 && size == -1))
            {
                _header = FByteBulkDataHeader(EBulkDataFlags::BULKDATA_None, size, static_cast<uint32_t>(size),
                                              offset, FBulkDataCookedIndex::Default());
            }
        }

        bool IsValid() const override { return File != nullptr; }

        std::vector<uint8_t> GetData() const override
        {
            if (File == nullptr) return {};
            auto data = File->SafeRead(_header.has_value() ? &*_header : nullptr);
            return data.has_value() ? std::move(*data) : std::vector<uint8_t>();
        }

    private:
        std::optional<FByteBulkDataHeader> _header;
    };

    // A sub-range of an export's bulk data, carved out with FByteBulkData's sub-range constructor.
    class FBulkDataDeferredByteData final : public FDeferredByteData
    {
    public:
        FByteBulkData BulkData;

        FBulkDataDeferredByteData(FAssetArchive& Ar, const FByteBulkData& bulkData, int64_t offset, int32_t size)
            : BulkData(Ar, bulkData, offset, size) {}

        // C# tests `BulkData is not null`, which a value member is always; the sub-range is valid when it
        // actually covers bytes.
        bool IsValid() const override { return BulkData.Header.SizeOnDisk > 0; }

        std::vector<uint8_t> GetData() const override
        {
            auto data = BulkData.ReadDataOnce();
            return data.has_value() ? std::move(*data) : std::vector<uint8_t>();
        }
    };
}
