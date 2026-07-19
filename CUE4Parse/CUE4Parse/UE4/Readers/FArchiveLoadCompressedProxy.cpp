// Ported from CUE4Parse/UE4/Readers/FArchiveLoadCompressedProxy.cs
#include "FArchiveLoadCompressedProxy.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

#include "../../Compression/Compression.h"
#include "../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Readers
{
    using Objects::Core::Misc::ECompressionFlags;
    static constexpr int LOADING_COMPRESSION_CHUNK_SIZE = CUE4Parse::Compression::Compression::LOADING_COMPRESSION_CHUNK_SIZE;

    FArchiveLoadCompressedProxy::FArchiveLoadCompressedProxy(std::string name, std::vector<uint8_t> compressedData,
                                                             std::string compressionFormat, ECompressionFlags flags,
                                                             VersionContainer versions)
        : FArchive(std::move(versions))
        , _name(std::move(name))
        , _compressedData(std::move(compressedData))
        , _tmpData(static_cast<size_t>(LOADING_COMPRESSION_CHUNK_SIZE))
        , _tmpDataPos(LOADING_COMPRESSION_CHUNK_SIZE)
        , _tmpDataSize(LOADING_COMPRESSION_CHUNK_SIZE)
        , _compressionFormat(std::move(compressionFormat))
        , _compressionFlags(flags)
    {
        // C#'s Length throws; here it is unknown up front, so we treat the stream as unbounded for
        // CheckReadSize purposes (see header note).
        Length = std::numeric_limits<int64_t>::max();
        Position = 0;
    }

    std::unique_ptr<FArchive> FArchiveLoadCompressedProxy::Clone() const
    {
        return std::make_unique<FArchiveLoadCompressedProxy>(_name, _compressedData, _compressionFormat, _compressionFlags, Versions);
    }

    int FArchiveLoadCompressedProxy::Read(uint8_t* dstData, int offset, int count)
    {
        if (_shouldSerializeFromArray)
        {
            // SerializedCompressed reads the compressed data from here.
            std::memcpy(dstData + offset, _compressedData.data() + _currentIndex, static_cast<size_t>(count));
            _currentIndex += count;
            return count;
        }

        // Regular call to serialize, read from temp buffer.
        int dstPos = 0;
        while (count > 0)
        {
            const int bytesToCopy = std::min(count, _tmpDataSize - _tmpDataPos);
            if (bytesToCopy > 0)
            {
                // A NULL destination is used for forward seeking: decompress but don't copy.
                if (dstData != nullptr)
                {
                    std::memcpy(dstData + offset + dstPos, _tmpData.data() + _tmpDataPos, static_cast<size_t>(bytesToCopy));
                    dstPos += bytesToCopy;
                }
                count -= bytesToCopy;
                _tmpDataPos += bytesToCopy;
                _rawBytesSerialized += bytesToCopy;
            }
            else
            {
                // Tmp buffer fully exhausted, decompress a new one (this re-enters Read via the array path).
                DecompressMoreData();
                if (_tmpDataSize == 0)
                {
                    // Wanted more but couldn't get any; avoid an infinite loop.
                    throw Exceptions::ParserException(*this, "FArchiveLoadCompressedProxy ran out of data");
                }
            }
        }

        Position = _rawBytesSerialized;
        return dstPos;
    }

    int64_t FArchiveLoadCompressedProxy::Seek(int64_t offset, ESeekOrigin origin)
    {
        if (origin != ESeekOrigin::Begin)
            throw Exceptions::ParserException(*this, "FArchiveLoadCompressedProxy only supports seeking from Begin");
        const int64_t difference = offset - Position;
        // We only support forward seeking.
        if (difference < 0)
            throw Exceptions::ParserException(*this, "FArchiveLoadCompressedProxy only supports forward seeking");
        // Seek by serializing data with a NULL destination so it just decompresses.
        Read(nullptr, 0, static_cast<int>(difference));
        return Position;
    }

    void FArchiveLoadCompressedProxy::DecompressMoreData()
    {
        // Indicate that SerializeCompressedNew should read the compressed source from _compressedData.
        _shouldSerializeFromArray = true;
        int64_t decompressedLength = 0;
        SerializeCompressedNew(_tmpData.data(), LOADING_COMPRESSION_CHUNK_SIZE, _compressionFormat, _compressionFlags,
                               false, decompressedLength);
        _shouldSerializeFromArray = false;
        // Buffer is filled again, reset.
        _tmpDataPos = 0;
        _tmpDataSize = static_cast<int>(decompressedLength);
    }
}
