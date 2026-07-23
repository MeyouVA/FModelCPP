// Ported from CUE4Parse/Compression/LZ4.cs — a safe LZ4 block decompressor.
#include "LZ4.h"

namespace CUE4Parse::Compression
{
    namespace
    {
        constexpr int MINMATCH = 4;
        constexpr int ML_BITS = 4;
        constexpr int ML_MASK = (1 << ML_BITS) - 1; // 0x0f
        constexpr int RUN_BITS = 8 - ML_BITS;
        constexpr int RUN_MASK = (1 << RUN_BITS) - 1; // 0x0f
        constexpr int LASTLITERALS = 5;
        constexpr int MFLIMIT = 12;
    }

    int LZ4_decompress_safe(const uint8_t* src, uint8_t* dst, int compressedSize, int dstCapacity)
    {
        if (compressedSize < 0 || dstCapacity < 0) return -1;

        const uint8_t* ip = src;
        const uint8_t* const iend = src + compressedSize;
        uint8_t* op = dst;
        uint8_t* const oend = dst + dstCapacity;

        if (compressedSize == 0) return dstCapacity == 0 ? 0 : -1;

        while (true)
        {
            // Token: high nibble = literal length, low nibble = (match length - MINMATCH).
            if (ip >= iend) return -1;
            const int token = *ip++;

            // ---- literals ----
            int length = token >> ML_BITS;
            if (length == RUN_MASK)
            {
                int s;
                do
                {
                    if (ip >= iend) return -1;
                    s = *ip++;
                    length += s;
                } while (s == 255);
            }

            // Copy literals.
            if (op + length > oend) return -1;       // output overrun
            if (ip + length > iend) return -1;       // input overrun
            for (int i = 0; i < length; i++)
                *op++ = *ip++;

            // End of the last sequence: literals only, no match follows.
            if (ip == iend)
                break;

            // ---- match ----
            // 2-byte little-endian offset.
            if (ip + 2 > iend) return -1;
            const int offset = ip[0] | (ip[1] << 8);
            ip += 2;
            if (offset == 0) return -1;

            const uint8_t* match = op - offset;
            if (match < dst) return -1;              // reference before the start of output

            int matchLength = token & ML_MASK;
            if (matchLength == ML_MASK)
            {
                int s;
                do
                {
                    if (ip >= iend) return -1;
                    s = *ip++;
                    matchLength += s;
                } while (s == 255);
            }
            matchLength += MINMATCH;

            // Copy match byte-by-byte (ranges may overlap when offset < matchLength).
            if (op + matchLength > oend) return -1;  // output overrun
            for (int i = 0; i < matchLength; i++)
                *op++ = *match++;
        }

        return static_cast<int>(op - dst);
    }
}
