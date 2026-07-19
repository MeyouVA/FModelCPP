// Ported from CUE4Parse/Utils/CRC32.cs
#include "CRC32.h"

namespace CUE4Parse::Utils
{
    CRC32::CRC32() : CRC32(false) {}

    CRC32::CRC32(bool reverseBits) : CRC32(static_cast<int32_t>(0xEDB88320), reverseBits) {}

    CRC32::CRC32(int32_t polynomial, bool reverseBits)
    {
        _reverseBits = reverseBits;
        _dwPolynomial = static_cast<uint32_t>(polynomial);
        GenerateLookupTable();
    }

    int32_t CRC32::GetCrc32(const std::vector<uint8_t>& input)
    {
        _totalBytesRead = 0;
        if (!input.empty())
            SlurpBlock(input.data(), 0, static_cast<int>(input.size()));
        return static_cast<int32_t>(~_register);
    }

    int32_t CRC32::ComputeCrc32(int32_t w, uint8_t b) const
    {
        uint32_t uw = static_cast<uint32_t>(w);
        return static_cast<int32_t>(_crc32Table[(uw ^ b) & 0xFF] ^ (uw >> 8));
    }

    void CRC32::SlurpBlock(const uint8_t* block, int offset, int count)
    {
        for (int i = 0; i < count; i++)
        {
            uint8_t b = block[offset + i];
            if (_reverseBits)
            {
                uint32_t temp = (_register >> 24) ^ b;
                _register = (_register << 8) ^ _crc32Table[temp];
            }
            else
            {
                uint32_t temp = (_register & 0x000000FF) ^ b;
                _register = (_register >> 8) ^ _crc32Table[temp];
            }
        }
        _totalBytesRead += count;
    }

    void CRC32::SlurpBlock(const std::vector<uint8_t>& block, int offset, int count)
    {
        SlurpBlock(block.data(), offset, count);
    }

    void CRC32::UpdateCRC(uint8_t b)
    {
        if (_reverseBits)
        {
            uint32_t temp = (_register >> 24) ^ b;
            _register = (_register << 8) ^ _crc32Table[temp];
        }
        else
        {
            uint32_t temp = (_register & 0x000000FF) ^ b;
            _register = (_register >> 8) ^ _crc32Table[temp];
        }
    }

    void CRC32::UpdateCRC(uint8_t b, int n)
    {
        while (n-- > 0)
        {
            if (_reverseBits)
            {
                uint32_t temp = (_register >> 24) ^ b;
                _register = (_register << 8) ^ _crc32Table[temp];
            }
            else
            {
                uint32_t temp = (_register & 0x000000FF) ^ b;
                _register = (_register >> 8) ^ _crc32Table[temp];
            }
        }
    }

    uint32_t CRC32::ReverseBits(uint32_t data)
    {
        uint32_t ret = data;
        ret = ((ret & 0x55555555) << 1) | ((ret >> 1) & 0x55555555);
        ret = ((ret & 0x33333333) << 2) | ((ret >> 2) & 0x33333333);
        ret = ((ret & 0x0F0F0F0F) << 4) | ((ret >> 4) & 0x0F0F0F0F);
        ret = (ret << 24) | ((ret & 0xFF00) << 8) | ((ret >> 8) & 0xFF00) | (ret >> 24);
        return ret;
    }

    uint8_t CRC32::ReverseBits(uint8_t data)
    {
        uint32_t u = static_cast<uint32_t>(data) * 0x00020202;
        uint32_t m = 0x01044010;
        uint32_t s = u & m;
        uint32_t t = (u << 2) & (m << 1);
        return static_cast<uint8_t>((0x01001001 * (s + t)) >> 24);
    }

    void CRC32::GenerateLookupTable()
    {
        uint8_t i = 0;
        do
        {
            uint32_t dwCrc = i;
            for (uint8_t j = 8; j > 0; j--)
            {
                if ((dwCrc & 1) == 1)
                    dwCrc = (dwCrc >> 1) ^ _dwPolynomial;
                else
                    dwCrc >>= 1;
            }
            if (_reverseBits)
                _crc32Table[ReverseBits(i)] = ReverseBits(dwCrc);
            else
                _crc32Table[i] = dwCrc;
            i++;
        } while (i != 0);
    }

    uint32_t CRC32::gf2_matrix_times(const uint32_t* matrix, uint32_t vec) const
    {
        uint32_t sum = 0;
        int i = 0;
        while (vec != 0)
        {
            if ((vec & 0x01) == 0x01)
                sum ^= matrix[i];
            vec >>= 1;
            i++;
        }
        return sum;
    }

    void CRC32::gf2_matrix_square(uint32_t* square, const uint32_t* mat) const
    {
        for (int i = 0; i < 32; i++)
            square[i] = gf2_matrix_times(mat, mat[i]);
    }

    void CRC32::Combine(int32_t crc, int32_t length)
    {
        uint32_t even[32] = {}; // even-power-of-two zeros operator
        uint32_t odd[32] = {};  // odd-power-of-two zeros operator

        if (length == 0)
            return;

        uint32_t crc1 = ~_register;
        uint32_t crc2 = static_cast<uint32_t>(crc);

        // put operator for one zero bit in odd
        odd[0] = _dwPolynomial; // the CRC-32 polynomial
        uint32_t row = 1;
        for (int i = 1; i < 32; i++)
        {
            odd[i] = row;
            row <<= 1;
        }

        gf2_matrix_square(even, odd);
        gf2_matrix_square(odd, even);

        uint32_t len2 = static_cast<uint32_t>(length);

        do
        {
            gf2_matrix_square(even, odd);
            if ((len2 & 1) == 1)
                crc1 = gf2_matrix_times(even, crc1);
            len2 >>= 1;

            if (len2 == 0)
                break;

            gf2_matrix_square(odd, even);
            if ((len2 & 1) == 1)
                crc1 = gf2_matrix_times(odd, crc1);
            len2 >>= 1;
        } while (len2 != 0);

        crc1 ^= crc2;
        _register = ~crc1;
    }

    void CRC32::Reset()
    {
        _register = 0xFFFFFFFFu;
    }
}
