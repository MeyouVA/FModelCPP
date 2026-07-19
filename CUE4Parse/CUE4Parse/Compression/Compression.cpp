// Ported from CUE4Parse/Compression/Compression.cs
#include "Compression.h"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

#include "../UE4/Exceptions/UnknownCompressionMethodException.h"

namespace CUE4Parse::Compression
{
    using CUE4Parse::UE4::Exceptions::UnknownCompressionMethodException;

    namespace
    {
        // One slot per CompressionAlgorithm value (Brotli is the highest, index 6).
        std::array<DecompressFunc, 7> g_decompressors{};

        int AlgorithmIndex(CompressionAlgorithm a) { return static_cast<int>(a); }

        // Maps a CompressionMethod to its algorithm; returns false for None (no algorithm).
        bool MapMethod(CompressionMethod method, CompressionAlgorithm& outAlgorithm, const UE4::Readers::FArchive* reader)
        {
            switch (method)
            {
                case CompressionMethod::None:
                    return false;
                case CompressionMethod::Zlib:
                case CompressionMethod::XB1Zlib:
                case CompressionMethod::XboxOneGDKZlib:
                    outAlgorithm = CompressionAlgorithm::Zlib;
                    return true;
                case CompressionMethod::Gzip:
                    outAlgorithm = CompressionAlgorithm::Gzip;
                    return true;
                case CompressionMethod::Oodle:
                    outAlgorithm = CompressionAlgorithm::Oodle;
                    return true;
                case CompressionMethod::LZ4:
                    outAlgorithm = CompressionAlgorithm::LZ4;
                    return true;
                case CompressionMethod::Brotli:
                    outAlgorithm = CompressionAlgorithm::Brotli;
                    return true;
                case CompressionMethod::Zstd:
                    outAlgorithm = CompressionAlgorithm::Zstd;
                    return true;
                default:
                {
                    const std::string msg = "Compression method \"" + std::to_string(static_cast<int>(method)) + "\" is unknown";
                    if (reader != nullptr)
                        throw UnknownCompressionMethodException(*reader, msg);
                    throw UnknownCompressionMethodException(msg);
                }
            }
        }
    }

    void Compression::RegisterDecompressor(CompressionAlgorithm algorithm, DecompressFunc func)
    {
        g_decompressors[AlgorithmIndex(algorithm)] = std::move(func);
    }

    bool Compression::HasDecompressor(CompressionAlgorithm algorithm)
    {
        if (algorithm == CompressionAlgorithm::None) return true;
        return static_cast<bool>(g_decompressors[AlgorithmIndex(algorithm)]);
    }

    std::vector<uint8_t> Compression::Decompress(const std::vector<uint8_t>& compressed, int uncompressedSize,
                                                 CompressionMethod method, const UE4::Readers::FArchive* reader)
    {
        return Decompress(compressed, 0, static_cast<int>(compressed.size()), uncompressedSize, method, reader);
    }

    std::vector<uint8_t> Compression::Decompress(const std::vector<uint8_t>& compressed, int compressedOffset, int compressedCount,
                                                 int uncompressedSize, CompressionMethod method, const UE4::Readers::FArchive* reader)
    {
        std::vector<uint8_t> uncompressed(static_cast<size_t>(uncompressedSize));
        Decompress(compressed, compressedOffset, compressedCount, uncompressed, 0, uncompressedSize, method, reader);
        return uncompressed;
    }

    void Compression::Decompress(const std::vector<uint8_t>& compressed, int compressedOffset, int compressedSize,
                                 std::vector<uint8_t>& uncompressed, int uncompressedOffset, int uncompressedSize,
                                 CompressionMethod method, const UE4::Readers::FArchive* reader)
    {
        Decompress(compressed.data() + compressedOffset, compressedSize,
                   uncompressed.data() + uncompressedOffset, uncompressedSize, method, reader);
    }

    void Compression::Decompress(const uint8_t* compressed, int compressedSize,
                                 uint8_t* uncompressed, int uncompressedSize,
                                 CompressionMethod method, const UE4::Readers::FArchive* reader)
    {
        CompressionAlgorithm algorithm;
        if (!MapMethod(method, algorithm, reader))
        {
            // None: straight copy (matches compressed.CopyTo(uncompressed)).
            std::memcpy(uncompressed, compressed, static_cast<size_t>(compressedSize));
            return;
        }

        const DecompressFunc& fn = g_decompressors[AlgorithmIndex(algorithm)];
        if (!fn)
        {
            const std::string msg = "No decompressor registered for algorithm \"" +
                std::to_string(static_cast<int>(algorithm)) + "\" (method " + std::to_string(static_cast<int>(method)) + ")";
            if (reader != nullptr)
                throw UnknownCompressionMethodException(*reader, msg);
            throw UnknownCompressionMethodException(msg);
        }

        int bytesWritten = 0;
        if (!fn(compressed, compressedSize, uncompressed, uncompressedSize, bytesWritten) || bytesWritten != uncompressedSize)
        {
            // C# throws FileLoadException here; we surface the same detail as a runtime_error.
            throw std::runtime_error("Failed to decompress " + std::to_string(static_cast<int>(method)) +
                " data (Expected: " + std::to_string(uncompressedSize) + ", Result: " + std::to_string(bytesWritten) + ")");
        }
    }
}
