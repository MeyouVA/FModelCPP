// A self-contained DEFLATE (RFC 1951) inflater plus zlib (RFC 1950) and gzip (RFC 1952) wrappers.
//
// The C# CUE4Parse leans on System.IO.Compression / the native Zlib-ng DLL for this; the C++ port has no
// managed BCL, so it ships its own decoder (a compact, canonical-Huffman inflate in the style of Mark Adler's
// reference "puff"). This makes Zlib/Gzip decompression fully built-in — no external library or DLL download.
//
// Each function writes into a caller-provided output buffer of known size and returns the number of bytes
// produced, or a negative value on error.
#pragma once

#include <cstdint>

namespace CUE4Parse::Compression
{
    // Raw DEFLATE stream (no header/trailer).
    int Inflate(const uint8_t* src, int srcLen, uint8_t* dst, int dstCap);

    // zlib-wrapped DEFLATE: 2-byte CMF/FLG header (+ optional preset dictionary) then DEFLATE, Adler-32 trailer.
    int ZlibDecompress(const uint8_t* src, int srcLen, uint8_t* dst, int dstCap);

    // gzip-wrapped DEFLATE: 10-byte header (+ optional extra/name/comment/hcrc fields) then DEFLATE, CRC32/ISIZE trailer.
    int GzipDecompress(const uint8_t* src, int srcLen, uint8_t* dst, int dstCap);
}
