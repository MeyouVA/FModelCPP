// Ported from CUE4Parse/Utils/CRC32.cs (originally DotNetZip, Microsoft Public License).
// The parameterized CRC-32 (polynomial + optional bit reversal) used for GZIP/BZip2/ZIP.
// The C# file also defines CrcCalculatorStream (a Stream wrapper); that is omitted here since the
// C++ port has no Stream base — feed bytes through SlurpBlock/UpdateCRC directly instead.
#pragma once

#include <cstdint>
#include <vector>

namespace CUE4Parse::Utils
{
    class CRC32
    {
    public:
        // Default: no bit reversal, polynomial 0xEDB88320.
        CRC32();
        explicit CRC32(bool reverseBits);
        CRC32(int32_t polynomial, bool reverseBits);

        int64_t TotalBytesRead() const { return _totalBytesRead; }
        int32_t Crc32Result() const { return static_cast<int32_t>(~_register); }

        // Compute the CRC32 over a whole buffer (equivalent to the Stream overload in C#).
        int32_t GetCrc32(const std::vector<uint8_t>& input);

        // PKZIP 2.0 weak-encryption (word, byte) combo.
        int32_t ComputeCrc32(int32_t w, uint8_t b) const;

        void SlurpBlock(const uint8_t* block, int offset, int count);
        void SlurpBlock(const std::vector<uint8_t>& block, int offset, int count);
        void UpdateCRC(uint8_t b);
        void UpdateCRC(uint8_t b, int n);

        // Combine another CRC32 (over `length` bytes) into this running total.
        void Combine(int32_t crc, int32_t length);

        void Reset();

    private:
        void GenerateLookupTable();
        uint32_t gf2_matrix_times(const uint32_t* matrix, uint32_t vec) const;
        void gf2_matrix_square(uint32_t* square, const uint32_t* mat) const;
        static uint32_t ReverseBits(uint32_t data);
        static uint8_t ReverseBits(uint8_t data);

        uint32_t _dwPolynomial = 0;
        int64_t _totalBytesRead = 0;
        bool _reverseBits = false;
        uint32_t _crc32Table[256] = {};
        uint32_t _register = 0xFFFFFFFFu;
    };
}
