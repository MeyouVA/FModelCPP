// Ported from CUE4Parse/Compression/Compression.cs
//
// C# builds a decompressor from external packages (Oodle, K4os LZ4, ZstdSharp, Zlib-ng) via a
// DecompressorBuilder. Those native codecs are not vendored into this C++ port yet, so instead of
// hard-wiring library calls, Decompress routes through a small registry: a per-algorithm decompress
// callback is registered once (mirroring C#'s builder .Add(algorithm, fn)). `None` always works
// (a plain copy); any other algorithm throws until a codec is registered via RegisterDecompressor.
// When the LZ4/Zstd/Oodle/Zlib layers are brought in, register them here and nothing else changes.
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "CompressionMethod.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::Compression
{
    // Mirrors the CompressionAlgorithm enum the C# DecompressorBuilder keys on.
    enum class CompressionAlgorithm : int32_t
    {
        None = 0,
        Oodle,
        LZ4,
        Zstd,
        Zlib,
        Gzip,
        Brotli
    };

    // A decompress callback: read `srcLen` bytes at `src`, write up to `dstLen` bytes to `dst`, and
    // report how many bytes were actually written. Return false on failure. Matches the C# delegate
    // shape `(source, destination, out written) => bool`.
    using DecompressFunc = std::function<bool(const uint8_t* src, int srcLen, uint8_t* dst, int dstLen, int& written)>;

    class Compression
    {
    public:
        static constexpr int LOADING_COMPRESSION_CHUNK_SIZE = 131072;

        // Register (or replace) the codec for an algorithm. Call once at startup per available codec,
        // as C#'s DecompressorBuilder.Add does. `None` is handled internally and need not be registered.
        static void RegisterDecompressor(CompressionAlgorithm algorithm, DecompressFunc func);

        // Registers the codecs that ship built into this port: LZ4, Zlib, and Gzip (the last two via the
        // in-tree Inflate decoder). Called automatically at startup, mirroring C#'s default DecompressorBuilder.
        // Oodle and Zstd are not built in — see OodleHelper / ZstdHelper to load their native libraries.
        static void RegisterBuiltinDecompressors();

        // True if a codec is available for the algorithm (or it is None).
        static bool HasDecompressor(CompressionAlgorithm algorithm);

        // Allocate-and-return overloads.
        static std::vector<uint8_t> Decompress(const std::vector<uint8_t>& compressed, int uncompressedSize,
                                               CompressionMethod method, const UE4::Readers::FArchive* reader = nullptr);
        static std::vector<uint8_t> Decompress(const std::vector<uint8_t>& compressed, int compressedOffset, int compressedCount,
                                               int uncompressedSize, CompressionMethod method, const UE4::Readers::FArchive* reader = nullptr);

        // Fill-provided-buffer overloads.
        static void Decompress(const std::vector<uint8_t>& compressed, int compressedOffset, int compressedSize,
                               std::vector<uint8_t>& uncompressed, int uncompressedOffset, int uncompressedSize,
                               CompressionMethod method, const UE4::Readers::FArchive* reader = nullptr);

        // Span-style core (raw pointers + lengths).
        static void Decompress(const uint8_t* compressed, int compressedSize,
                               uint8_t* uncompressed, int uncompressedSize,
                               CompressionMethod method, const UE4::Readers::FArchive* reader = nullptr);
    };
}
