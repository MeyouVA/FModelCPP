// Ported from CUE4Parse/UE4/Objects/Core/Math/FFloat16.cs.
// C#'s FFloat16 is a class holding the raw IEEE-754 half encoding; here it is a small value struct.
//
// Addition with no C# counterpart: ToFloat/FromFloat. C# never decodes FFloat16 because it has System.Half and
// uses *that* wherever a half value is actually consumed (FHalfVector, FVector3UnsignedShort, ...). C++ has no
// standard half, so FFloat16 doubles as the Half stand-in and has to carry the conversion itself.
#pragma once

#include <cstdint>
#include <cstring>

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FFloat16
    {
        uint16_t Encoded = 0;

        FFloat16() = default;
        explicit FFloat16(CUE4Parse::UE4::Readers::FArchive& Ar) { Encoded = Ar.Read<uint16_t>(); }

        // IEEE-754 binary16 → binary32. Exact for every input: binary32 can represent every half value,
        // including subnormals, infinities and NaNs.
        float ToFloat() const
        {
            const uint32_t sign = static_cast<uint32_t>(Encoded & 0x8000u) << 16;
            const uint32_t exponent = (Encoded >> 10) & 0x1Fu;
            const uint32_t mantissa = Encoded & 0x3FFu;

            uint32_t bits;
            if (exponent == 0)
            {
                if (mantissa == 0)
                {
                    bits = sign; // +/- zero
                }
                else
                {
                    // Subnormal half: renormalise it into a normal float, which always has the range for it.
                    uint32_t e = 0;
                    uint32_t m = mantissa;
                    while ((m & 0x400u) == 0) { m <<= 1; ++e; } // stops with the implicit bit in place
                    bits = sign | ((127 - 15 + 1 - e) << 23) | ((m & 0x3FFu) << 13);
                }
            }
            else if (exponent == 0x1Fu)
            {
                bits = sign | 0x7F800000u | (mantissa << 13); // infinity / NaN
            }
            else
            {
                bits = sign | ((exponent + (127 - 15)) << 23) | (mantissa << 13);
            }

            float result;
            std::memcpy(&result, &bits, sizeof(result));
            return result;
        }

        // Explicit, like C#'s (float) cast off System.Half — an implicit one would quietly pull FFloat16 into
        // arithmetic overload resolution everywhere.
        explicit operator float() const { return ToFloat(); }

        // binary32 → binary16, round-to-nearest-even, matching System.Half's narrowing conversion.
        static FFloat16 FromFloat(float value)
        {
            uint32_t bits;
            std::memcpy(&bits, &value, sizeof(bits));

            const uint32_t sign = (bits >> 16) & 0x8000u;
            const int32_t exponent = static_cast<int32_t>((bits >> 23) & 0xFFu) - 127 + 15;
            const uint32_t mantissa = bits & 0x7FFFFFu;

            FFloat16 result;
            if (((bits >> 23) & 0xFFu) == 0xFFu)
            {
                // Infinity or NaN. Keep a NaN a NaN even if its payload would truncate to zero.
                result.Encoded = static_cast<uint16_t>(sign | 0x7C00u | (mantissa != 0 ? 0x200u : 0u));
            }
            else if (exponent >= 0x1F)
            {
                result.Encoded = static_cast<uint16_t>(sign | 0x7C00u); // overflow to infinity
            }
            else if (exponent <= 0)
            {
                if (exponent < -10)
                {
                    result.Encoded = static_cast<uint16_t>(sign); // underflow to zero
                }
                else
                {
                    const uint32_t m = mantissa | 0x800000u;
                    const int32_t shift = 14 - exponent;
                    uint32_t half = m >> shift;
                    // round half to even
                    const uint32_t rest = m & ((1u << shift) - 1u);
                    const uint32_t halfway = 1u << (shift - 1);
                    if (rest > halfway || (rest == halfway && (half & 1u) != 0)) ++half;
                    result.Encoded = static_cast<uint16_t>(sign | half);
                }
            }
            else
            {
                uint32_t half = (static_cast<uint32_t>(exponent) << 10) | (mantissa >> 13);
                const uint32_t rest = mantissa & 0x1FFFu;
                if (rest > 0x1000u || (rest == 0x1000u && (half & 1u) != 0)) ++half;
                result.Encoded = static_cast<uint16_t>(sign | half);
            }
            return result;
        }
    };
}
