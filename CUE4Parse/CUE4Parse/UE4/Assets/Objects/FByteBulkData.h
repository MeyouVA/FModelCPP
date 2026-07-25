// Ported from CUE4Parse/UE4/Assets/Objects/FByteBulkData.cs
// The byte specialisation of TBulkData: what every audio/texture payload actually is. Overrides the read
// so it skips the element-array round trip, and adds the sub-range constructor the Wwise media layer uses
// to carve one .bnk or .wem out of a packed bulk blob.
//
// Deliberate differences from C#:
//   * TryCreateReader returns the archive (null on failure) instead of a bool + out param, and does not
//     log -- the port has no logging layer. It still enforces C#'s "empty reader counts as failure" rule.
//   * ReadDataOnce returns by value. C#'s `returnCachedData` flag exists to avoid handing a caller the
//     cached array it might mutate; a returned vector is already a copy, so the flag only decides whether
//     the *cache* is consulted at all, which is what the name means at every call site.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "TBulkData.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    /// Custom wrapper class for a bulk byte[] data without FByteBulkDataHeader
    class FByteArrayData final : public TBulkData<uint8_t>
    {
    public:
        explicit FByteArrayData(std::vector<uint8_t> data) : TBulkData<uint8_t>(std::move(data)) {}

        int GetDataSize() const override
        {
            const std::vector<uint8_t>* data = Data();
            return data != nullptr ? static_cast<int>(data->size()) : 0;
        }
    };

    class FByteBulkData final : public TBulkData<uint8_t>
    {
    public:
        FByteBulkData() = default;
        explicit FByteBulkData(FAssetArchive& Ar) : TBulkData<uint8_t>(Ar) {}

        /// Creates a new FByteBulkData instance for a portion of the original bulk data.
        FByteBulkData(FAssetArchive& Ar, const FByteBulkData& bulkData, int64_t offset, int32_t size)
        {
            const FByteBulkDataHeader& header = bulkData.Header;
            Header = FByteBulkDataHeader(header.BulkDataFlags, size, static_cast<uint32_t>(size),
                                         header.OffsetInFile + offset, header.CookedIndex);
            _dataPosition = bulkData._dataPosition;
            // When the payload is elsewhere the offset is already folded into OffsetInFile above; only an
            // inline payload also needs the in-archive cursor moved.
            if (!HasFlag(header.BulkDataFlags,
                         EBulkDataFlags::BULKDATA_OptionalPayload | EBulkDataFlags::BULKDATA_PayloadInSeperateFile |
                         EBulkDataFlags::BULKDATA_PayloadAtEndOfFile))
            {
                _dataPosition += offset;
            }

            if (Header.SizeOnDisk == 0 || HasFlag(BulkDataFlags(), EBulkDataFlags::BULKDATA_Unused))
            {
                _readsEmpty = true;
                return;
            }

            _savedAr = &Ar;
        }

        int GetDataSize() const override { return Header.ElementCount; }

        /// Reads bulk data once without storing it in this instance.
        /// If data is already cached, optionally returns a copy of a cached data.
        std::optional<std::vector<uint8_t>> ReadDataOnce(bool returnCachedData = true) const
        {
            if (returnCachedData && _dataEvaluated)
            {
                if (!_data.has_value()) return std::nullopt;
                return *_data;
            }

            std::vector<uint8_t> data;
            if (!ReadBulkDataInto(data)) return std::nullopt;
            return data;
        }

        // C#'s TryCreateReader: null when the read threw or produced nothing.
        std::unique_ptr<Readers::FArchive> TryCreateReader(const std::string& name, bool useCachedData = true) const
        {
            std::unique_ptr<Readers::FArchive> reader;
            try
            {
                std::optional<std::vector<uint8_t>> data = ReadDataOnce(useCachedData);
                reader = std::make_unique<FByteArchive>(name, data.has_value() ? std::move(*data) : std::vector<uint8_t>(),
                                                        _savedAr != nullptr ? _savedAr->Versions : VersionContainer());
            }
            catch (...)
            {
                // C# logs the exception here; the port has no logging layer.
                return nullptr;
            }
            return reader->Length > 0 ? std::move(reader) : nullptr;
        }

        // Reads the *next* bulk-data record off Ar and stitches the two together. Some cooked banks split a
        // payload in two; when the second chunk already begins with the first, it IS the whole thing.
        bool TryCombineBulkData(FAssetArchive& Ar, std::vector<uint8_t>& combinedData,
                                std::unique_ptr<FByteBulkData>& fullBulkData) const
        {
            fullBulkData.reset();
            combinedData.clear();
            const int64_t saved = Ar.Position;
            try
            {
                auto secondChunk = std::make_unique<FByteBulkData>(Ar);
                std::optional<std::vector<uint8_t>> secondChunkData = secondChunk->ReadDataOnce();
                std::optional<std::vector<uint8_t>> data = ReadDataOnce();
                if (!data.has_value() || !secondChunkData.has_value()) return false;

                if (data->size() < secondChunkData->size() &&
                    std::equal(data->begin(), data->end(), secondChunkData->begin()))
                {
                    combinedData = std::move(*secondChunkData);
                    fullBulkData = std::move(secondChunk);
                    return true;
                }

                combinedData.reserve(data->size() + secondChunkData->size());
                combinedData.insert(combinedData.end(), data->begin(), data->end());
                combinedData.insert(combinedData.end(), secondChunkData->begin(), secondChunkData->end());
                return true;
            }
            catch (...)
            {
                Ar.Position = saved;
                return false;
            }
        }

    protected:
        bool ReadBulkDataInto(std::vector<uint8_t>& data) const override
        {
            data.clear();

            BulkArchiveRef ref = GetBulkArchive();
            if (!ref) return false;

            data.assign(static_cast<size_t>(Header.SizeOnDisk), 0);
            // C# warns on a short read; the port keeps whatever came back, as C# does.
            ref.Archive->ReadAt(ref.Position, data.data(), 0, static_cast<int>(Header.SizeOnDisk));

            if (HasFlag(BulkDataFlags(), EBulkDataFlags::BULKDATA_SerializeCompressedZLIB))
            {
                std::vector<uint8_t> uncompressedData(static_cast<size_t>(Header.ElementCount));
                FByteArchive dataAr("", std::move(data),
                                    _savedAr != nullptr ? _savedAr->Versions : VersionContainer());
                int64_t partialReadLength = 0; // C#'s `out _`
                dataAr.SerializeCompressedNew(uncompressedData.data(), GetDataSize(), "Zlib",
                                              ECompressionFlags::COMPRESS_NoFlags, false, partialReadLength);
                data = std::move(uncompressedData);
            }

            return true;
        }
    };
}
