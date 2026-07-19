// Ported from CUE4Parse/UE4/Readers/FArchive.cs (non-template method bodies).
#include "FArchive.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#include "../../Compression/Compression.h"
#include "../../Compression/CompressionMethod.h"
#include "../Objects/UObject/FPackageFileSummary.h"

namespace CUE4Parse::UE4::Readers
{
    namespace
    {
        // Latin-1 (ISO-8859-1) -> UTF-8. Each input byte is a Unicode code point in [0, 255].
        // C# stores strings as UTF-16; we store UTF-8, so bytes >= 0x80 expand to two bytes.
        std::string Latin1ToUtf8(const uint8_t* data, size_t len)
        {
            std::string out;
            out.reserve(len);
            for (size_t i = 0; i < len; i++)
            {
                const uint8_t c = data[i];
                if (c < 0x80) { out.push_back(static_cast<char>(c)); }
                else
                {
                    out.push_back(static_cast<char>(0xC0 | (c >> 6)));
                    out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
                }
            }
            return out;
        }

        // UTF-16LE -> UTF-8 (handles surrogate pairs). len is the number of 16-bit code units.
        std::string Utf16LeToUtf8(const uint8_t* bytes, size_t units)
        {
            std::string out;
            out.reserve(units);
            for (size_t i = 0; i < units; i++)
            {
                uint32_t cp = static_cast<uint32_t>(bytes[i * 2]) | (static_cast<uint32_t>(bytes[i * 2 + 1]) << 8);
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < units)
                {
                    const uint32_t lo = static_cast<uint32_t>(bytes[(i + 1) * 2]) | (static_cast<uint32_t>(bytes[(i + 1) * 2 + 1]) << 8);
                    if (lo >= 0xDC00 && lo <= 0xDFFF)
                    {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        i++;
                    }
                }

                if (cp < 0x80) { out.push_back(static_cast<char>(cp)); }
                else if (cp < 0x800)
                {
                    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
                else if (cp < 0x10000)
                {
                    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
                else
                {
                    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
            }
            return out;
        }
    }

    bool FArchive::ReadBoolean()
    {
        const int32_t i = Read<int32_t>();
        switch (i)
        {
            case 0: return false;
            case 1: return true;
            default: throw Exceptions::ParserException(*this, "Invalid bool value (" + std::to_string(i) + ")");
        }
    }

    bool FArchive::ReadFlag()
    {
        const uint8_t i = Read<uint8_t>();
        switch (i)
        {
            case 0: return false;
            case 1: return true;
            default: throw Exceptions::ParserException(*this, "Invalid bool value (" + std::to_string(i) + ")");
        }
    }

    uint32_t FArchive::ReadIntPacked()
    {
        uint32_t value = 0;
        uint8_t cnt = 0;
        bool more = true;
        while (more)
        {
            uint8_t nextByte = Read<uint8_t>();          // Read next byte
            more = (nextByte & 1) != 0;                  // Low bit signals whether more follows
            nextByte = static_cast<uint8_t>(nextByte >> 1); // Shift to get the actual 7-bit value
            value += static_cast<uint32_t>(nextByte) << (7 * cnt++);
        }
        return value;
    }

    int FArchive::Read7BitEncodedInt()
    {
        int count = 0, shift = 0;
        uint8_t b;
        do
        {
            if (shift == 5 * 7) // 5 bytes max per Int32
                throw std::runtime_error("Stream is corrupted");
            b = Read<uint8_t>();
            count |= (b & 0x7F) << shift;
            shift += 7;
        } while ((b & 0x80) != 0);
        return count;
    }

    std::string FArchive::ReadString()
    {
        const int length = Read7BitEncodedInt();
        if (length <= 0) return std::string();
        auto bytes = ReadBytes(length);
        return Latin1ToUtf8(bytes.data(), static_cast<size_t>(length));
    }

    void FArchive::SkipFString()
    {
        const int32_t length = Read<int32_t>();
        if (length == INT32_MIN)
            throw std::out_of_range("Archive is corrupted");
        const int64_t strlength = length >= 0 ? length : -static_cast<int64_t>(length) * sizeof(uint16_t);
        if (strlength > Length - Position)
            throw Exceptions::ParserException("Invalid FString length '" + std::to_string(length) + "'");
        Position += strlength;
    }

    std::string FArchive::ReadFString()
    {
        // > 0 for ANSICHAR, < 0 for UCS2CHAR serialization
        int32_t length = Read<int32_t>();
        if (length == INT32_MIN)
            throw std::out_of_range("Archive is corrupted");
        if (std::abs(static_cast<int64_t>(length)) > Length - Position)
            throw Exceptions::ParserException("Invalid FString length '" + std::to_string(length) + "'");
        if (length == 0) return std::string();

        if (length < 0) // UCS2, 16-bit, fixed-width
        {
            length = -length;
            const int ucs2Length = length * static_cast<int>(sizeof(uint16_t));
            auto bytes = ReadBytes(ucs2Length);
            if (bytes[ucs2Length - 1] != 0 || bytes[ucs2Length - 2] != 0)
                throw Exceptions::ParserException(*this, "Serialized FString is not null terminated");
            // length - 1 drops the null terminator.
            return Utf16LeToUtf8(bytes.data(), static_cast<size_t>(length - 1));
        }

        auto bytes = ReadBytes(length);
        if (bytes[length - 1] != 0)
            throw Exceptions::ParserException(*this, "Serialized FString is not null terminated");
        return Latin1ToUtf8(bytes.data(), static_cast<size_t>(length - 1));
    }

    std::string FArchive::ReadFUtf8String() { return ReadFUtf8String(Read<int32_t>()); }
    std::string FArchive::ReadFUtf8String(int length)
    {
        if (length == 0) return std::string();
        if (length < 0) throw Exceptions::ParserException("Negative Utf8String length '" + std::to_string(length) + "'");
        if (length > Length - Position) throw Exceptions::ParserException("Invalid Utf8String length '" + std::to_string(length) + "'");
        auto span = ReadSpan(length);
        return std::string(reinterpret_cast<const char*>(span.data()), static_cast<size_t>(length));
    }

    std::string FArchive::ReadFAnsiString() { return ReadFAnsiString(Read<int32_t>()); }
    std::string FArchive::ReadFAnsiString(int length)
    {
        if (length == 0) return std::string();
        if (length < 0) throw Exceptions::ParserException("Negative AnsiString length '" + std::to_string(length) + "'");
        if (length > Length - Position) throw Exceptions::ParserException("Invalid AnsiString length '" + std::to_string(length) + "'");
        auto span = ReadSpan(length);
        return Latin1ToUtf8(span.data(), static_cast<size_t>(length));
    }

    float FArchive::ReadFReal()
    {
        return Ver() >= EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES
                   ? static_cast<float>(Read<double>())
                   : Read<float>();
    }

    FName FArchive::ReadFName() { return FName(ReadFString()); }

    void FArchive::CheckReadSize(int length)
    {
        if (length < 0)
            throw Exceptions::VersionException(*this, "Read size is smaller than zero.");
        if (Position + length > Length)
            throw Exceptions::VersionException(*this, "Read size is bigger than remaining archive length.");
    }

    namespace
    {
        using CUE4Parse::Compression::CompressionMethod;

        // 64-bit byte swap, matching C#'s BYTESWAP_ORDER64.
        inline uint64_t ByteswapOrder64(uint64_t value)
        {
            value = ((value << 8) & 0xFF00FF00FF00FF00ULL) | ((value >> 8) & 0x00FF00FF00FF00FFULL);
            value = ((value << 16) & 0xFFFF0000FFFF0000ULL) | ((value >> 16) & 0x0000FFFF0000FFFFULL);
            return (value << 32) | (value >> 32);
        }

        // Mirrors C#'s Enum.TryParse<CompressionMethod> (case-sensitive, by member name).
        bool TryParseCompressionMethod(const std::string& s, CompressionMethod& out)
        {
            if (s == "None") { out = CompressionMethod::None; return true; }
            if (s == "Zlib") { out = CompressionMethod::Zlib; return true; }
            if (s == "Gzip") { out = CompressionMethod::Gzip; return true; }
            if (s == "Custom") { out = CompressionMethod::Custom; return true; }
            if (s == "Oodle") { out = CompressionMethod::Oodle; return true; }
            if (s == "LZ4") { out = CompressionMethod::LZ4; return true; }
            if (s == "LZO") { out = CompressionMethod::LZO; return true; }
            if (s == "Zstd") { out = CompressionMethod::Zstd; return true; }
            if (s == "XB1Zlib") { out = CompressionMethod::XB1Zlib; return true; }
            if (s == "XboxOneGDKZlib") { out = CompressionMethod::XboxOneGDKZlib; return true; }
            if (s == "Brotli") { out = CompressionMethod::Brotli; return true; }
            if (s == "PWC") { out = CompressionMethod::PWC; return true; }
            if (s == "Unknown") { out = CompressionMethod::Unknown; return true; }
            return false;
        }
    }

    void FArchive::SerializeCompressedNew(uint8_t* dest, int length, const std::string& compressionFormatToDecodeOldV1Files,
                                          Objects::Core::Misc::ECompressionFlags flags, bool bTreatBufferAsFileReader,
                                          int64_t& outPartialReadLength)
    {
        using Summary = Objects::UObject::FPackageFileSummary;
        using Compressor = CUE4Parse::Compression::Compression;

        // Serialize package file tag used to determine endianess.
        FCompressedChunkInfo packageFileTag(*this);

        // v1 header did not store CompressionFormatToDecode; assume the passed-in old-files format (usually Zlib).
        std::string compressionFormatToDecode = compressionFormatToDecodeOldV1Files;

        bool bWasByteSwapped = false;
        bool bReadCompressionFormat = false;

        // low 32 bits of ARCHIVE_V2_HEADER_TAG are == PACKAGE_FILE_TAG
        const uint64_t ARCHIVE_V2_HEADER_TAG = Summary::PACKAGE_FILE_TAG | (static_cast<uint64_t>(0x22222222) << 32);

        if (packageFileTag.CompressedSize == static_cast<int64_t>(Summary::PACKAGE_FILE_TAG))
        {
            // v1 header, not swapped
        }
        else if (packageFileTag.CompressedSize == static_cast<int64_t>(Summary::PACKAGE_FILE_TAG_SWAPPED) ||
                 packageFileTag.CompressedSize == static_cast<int64_t>(ByteswapOrder64(Summary::PACKAGE_FILE_TAG)))
        {
            // v1 header, swapped
            bWasByteSwapped = true;
        }
        else if (packageFileTag.CompressedSize == static_cast<int64_t>(ARCHIVE_V2_HEADER_TAG) ||
                 packageFileTag.CompressedSize == static_cast<int64_t>(ByteswapOrder64(ARCHIVE_V2_HEADER_TAG)))
        {
            // v2 header
            bWasByteSwapped = packageFileTag.CompressedSize != static_cast<int64_t>(ARCHIVE_V2_HEADER_TAG);
            bReadCompressionFormat = true;

            switch (Read<uint8_t>())
            {
                case 0: compressionFormatToDecode = ReadFString(); break;
                case 1: compressionFormatToDecode = "None"; break;
                case 2: compressionFormatToDecode = "Oodle"; break;
                case 3: compressionFormatToDecode = "Zlib"; break;
                case 4: compressionFormatToDecode = "Gzip"; break;
                case 5: compressionFormatToDecode = "LZ4"; break;
                default:
                    throw Exceptions::ParserException(*this, "Unknown CompressionFormatToDecode value: " + compressionFormatToDecode);
            }
        }
        else
        {
            throw Exceptions::ParserException(*this, "BulkData compressed header read error. This package may be corrupt!");
        }

        if (!bReadCompressionFormat)
        {
            // upgrade old flag method
            if ((flags & Objects::Core::Misc::COMPRESS_DeprecatedFormatFlagsMask) != 0)
            {
                // C# logs a warning then throws NotImplementedException for deprecated flag-based formats.
                throw Exceptions::ParserException(*this, "Old style compression flags are not supported");
            }
        }

        // CompressionFormatToDecode came from disk, need to validate it.
        CompressionMethod compressionFormat;
        if (!TryParseCompressionMethod(compressionFormatToDecode, compressionFormat))
        {
            throw Exceptions::ParserException(
                *this, "BulkData compressed header read error. This package may be corrupt!\nCompressionFormatToDecode not found : " + compressionFormatToDecode);
        }

        // Read in base summary, contains total sizes. Note: C#'s Read<FCompressedChunkInfo>() blits 16 bytes.
        FCompressedChunkInfo summary = Read<FCompressedChunkInfo>();

        if (bWasByteSwapped)
        {
            summary.CompressedSize = static_cast<int64_t>(ByteswapOrder64(static_cast<uint64_t>(summary.CompressedSize)));
            summary.UncompressedSize = static_cast<int64_t>(ByteswapOrder64(static_cast<uint64_t>(summary.UncompressedSize)));
            packageFileTag.UncompressedSize = static_cast<int64_t>(ByteswapOrder64(static_cast<uint64_t>(packageFileTag.UncompressedSize)));
        }

        // Handle change in compression chunk size in a backward compatible way.
        int64_t loadingCompressionChunkSize = packageFileTag.UncompressedSize;
        if (loadingCompressionChunkSize == static_cast<int64_t>(Summary::PACKAGE_FILE_TAG))
            loadingCompressionChunkSize = Compressor::LOADING_COMPRESSION_CHUNK_SIZE;

        // check Summary.UncompressedSize vs [V,Length] passed in (UncompressedSize smaller than length is okay).
        if (summary.UncompressedSize > length)
            throw Exceptions::ParserException(
                *this, "Archive SerializedCompressed UncompressedSize (" + std::to_string(summary.UncompressedSize) +
                ") > Length (" + std::to_string(length) + ")");
        outPartialReadLength = summary.UncompressedSize;

        // Figure out how many chunks there are based on uncompressed size and compression chunk size.
        const int64_t totalChunkCount = (summary.UncompressedSize + loadingCompressionChunkSize - 1) / loadingCompressionChunkSize;

        std::vector<FCompressedChunkInfo> compressionChunks(static_cast<size_t>(totalChunkCount));
        int64_t maxCompressedSize = 0;
        int64_t totalChunkCompressedSize = 0;
        int64_t totalChunkUncompressedSize = 0;
        for (int64_t chunkIndex = 0; chunkIndex < totalChunkCount; chunkIndex++)
        {
            FCompressedChunkInfo chunk(*this);
            if (bWasByteSwapped)
            {
                chunk.CompressedSize = static_cast<int64_t>(ByteswapOrder64(static_cast<uint64_t>(chunk.CompressedSize)));
                chunk.UncompressedSize = static_cast<int64_t>(ByteswapOrder64(static_cast<uint64_t>(chunk.UncompressedSize)));
            }
            compressionChunks[static_cast<size_t>(chunkIndex)] = chunk;
            maxCompressedSize = std::max(chunk.CompressedSize, maxCompressedSize);
            totalChunkCompressedSize += chunk.CompressedSize;
            totalChunkUncompressedSize += chunk.UncompressedSize;
        }

        // Verify the CompressionChunks[] sizes we read add up to the total we read.
        if (totalChunkCompressedSize != summary.CompressedSize)
            throw Exceptions::ParserException(
                *this, "Archive SerializedCompressed TotalChunkCompressedSize (" + std::to_string(totalChunkCompressedSize) +
                ") != Summary.CompressedSize (" + std::to_string(summary.CompressedSize) + ")");
        if (totalChunkUncompressedSize != summary.UncompressedSize)
            throw Exceptions::ParserException(
                *this, "Archive SerializedCompressed TotalChunkUncompressedSize (" + std::to_string(totalChunkUncompressedSize) +
                ") != Summary.UncompressedSize (" + std::to_string(summary.UncompressedSize) + ")");

        // Set up destination pointer and allocate memory for compressed chunk[s] (one at a time).
        (void) bTreatBufferAsFileReader; // C# asserts this is false; only the in-memory path is supported.
        int destPos = 0;
        std::vector<uint8_t> compressedBuffer(static_cast<size_t>(maxCompressedSize));

        for (int64_t chunkIndex = 0; chunkIndex < totalChunkCount; chunkIndex++)
        {
            const FCompressedChunkInfo& chunk = compressionChunks[static_cast<size_t>(chunkIndex)];
            // Read compressed data.
            Read(compressedBuffer.data(), 0, static_cast<int>(chunk.CompressedSize));

            // Decompress into dest pointer directly.
            try
            {
                Compressor::Decompress(compressedBuffer.data(), static_cast<int>(chunk.CompressedSize),
                                       dest + destPos, static_cast<int>(chunk.UncompressedSize), compressionFormat, this);
            }
            catch (const std::exception& e)
            {
                throw Exceptions::ParserException(
                    *this, "Failed to uncompress data in " + Name() + ", CompressionFormatToDecode=" + compressionFormatToDecode +
                    " (" + e.what() + ")");
            }

            // And advance it by read amount.
            destPos += static_cast<int>(chunk.UncompressedSize);
        }
    }
}
