// Ported from CUE4Parse/UE4/Objects/Core/Compression/FCompressedBufferHeader.cs
// The 64-byte header in front of a compressed buffer payload. Every multi-byte field is stored big-endian
// (this is one of the few UE structures that is), so each read is byte-swapped on the way in.
//
// Deliberate differences from C#:
//   * The [JsonConverter] on Method is dropped -- the port has no JSON serializer layer.
//   * ReverseEndianness is a local helper rather than System.Buffers.Binary, matching FModGuid.h.
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Compression
{
    struct FCompressedBufferHeader
    {
        static constexpr uint32_t ExpectedMagic = 0xb7756362u;

        enum class EMethod : uint8_t
        {
            /** Header is followed by one uncompressed block. */
            None = 0,
            /** Header is followed by an array of compressed block sizes then the compressed blocks. */
            Oodle = 3,
            /** Header is followed by an array of compressed block sizes then the compressed blocks. */
            LZ4 = 4,
        };

        /** A magic number to identify a compressed buffer. Always 0xb7756362. */
        uint32_t Magic = ExpectedMagic;
        /** A CRC-32 used to check integrity of the buffer. Uses the polynomial 0x04c11db7. */
        uint32_t Crc32 = 0;
        /** The method used to compress the buffer. Affects layout of data following the header. */
        EMethod Method = EMethod::None;
        /** The method-specific compressor used to compress the buffer. */
        uint8_t Compressor = 0;
        /** The method-specific compression level used to compress the buffer. */
        uint8_t CompressionLevel = 0;
        /** The power of two size of every uncompressed block except the last. Size is 1 << BlockSizeExponent. */
        uint8_t BlockSizeExponent = 0;
        /** The number of blocks that follow the header. */
        uint32_t BlockCount = 0;
        /** The total size of the uncompressed data. */
        uint64_t TotalRawSize = 0;
        /** The total size of the compressed data including the header. */
        uint64_t TotalCompressedSize = 0;
        /** The hash of the uncompressed data. */
        std::vector<uint8_t> RawHash;

        FCompressedBufferHeader() = default;

        explicit FCompressedBufferHeader(Readers::FArchive& Ar)
        {
            Magic = ReverseEndianness(Ar.Read<uint32_t>());
            if (Magic != ExpectedMagic)
                throw std::runtime_error("FCompressedBuffer has invalid magic number: 0x" + ToHex8(Magic));

            Crc32 = ReverseEndianness(Ar.Read<uint32_t>());
            Method = Ar.Read<EMethod>();
            Compressor = Ar.Read<uint8_t>();
            CompressionLevel = Ar.Read<uint8_t>();
            BlockSizeExponent = Ar.Read<uint8_t>();
            BlockCount = ReverseEndianness(Ar.Read<uint32_t>());
            TotalRawSize = ReverseEndianness(Ar.Read<uint64_t>());
            TotalCompressedSize = ReverseEndianness(Ar.Read<uint64_t>());
            RawHash = Ar.ReadArray<uint8_t>(32);
        }

    private:
        static uint32_t ReverseEndianness(uint32_t v)
        {
            return (v >> 24) | ((v >> 8) & 0x0000FF00u) | ((v << 8) & 0x00FF0000u) | (v << 24);
        }

        static uint64_t ReverseEndianness(uint64_t v)
        {
            return (static_cast<uint64_t>(ReverseEndianness(static_cast<uint32_t>(v))) << 32) |
                   ReverseEndianness(static_cast<uint32_t>(v >> 32));
        }

        // C#'s $"0x{Magic:X8}" for the exception message.
        static std::string ToHex8(uint32_t v)
        {
            static const char* digits = "0123456789ABCDEF";
            std::string s(8, '0');
            for (int i = 7; i >= 0; --i) { s[static_cast<size_t>(i)] = digits[v & 0xFu]; v >>= 4; }
            return s;
        }
    };
}
