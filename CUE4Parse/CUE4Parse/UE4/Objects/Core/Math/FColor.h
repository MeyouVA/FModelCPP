// Ported from CUE4Parse/UE4/Objects/Core/Math/FColor.cs
// An 8-bit-per-channel (gamma-space) color. Field order matches the C# sequential layout (B, G, R, A).
//
// Deliberate differences from C#:
//   * Serialize(FArchiveWriter) is omitted — the Writers layer is not ported yet. TODO.
//   * The implicit FColor -> Vector4 conversion is omitted (System.Numerics; no consumer in the port yet).
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "../../../../Utils/UnsafePrint.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FColor
    {
        uint8_t B = 0;
        uint8_t G = 0;
        uint8_t R = 0;
        uint8_t A = 0;

        FColor() = default;
        explicit FColor(uint8_t b) : FColor(b, b, b, 255) {}
        FColor(uint8_t r, uint8_t g, uint8_t b) : FColor(r, g, b, 255) {}
        FColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : B(b), G(g), R(r), A(a) {}

        std::string Hex() const
        {
            return A == 255 || A == 0
                ? CUE4Parse::Utils::UnsafePrint::BytesToHex({R, G, B})
                : CUE4Parse::Utils::UnsafePrint::BytesToHex({A, R, G, B});
        }

        std::string ToString() const { return Hex(); }

        static uint8_t Requantize16to8(int value16)
        {
            if (value16 < 0 || value16 > 65535)
                throw std::invalid_argument("value16");

            // Dequantize from 16-bit then requantize to 8-bit with rounding (GPU UNorm convention);
            // matches (int)((value16/65535.f) * 255.f + 0.5f).
            const int value8 = (value16 * 255 + 32895) >> 16;
            return static_cast<uint8_t>(value8);
        }

        int ToPackedARGB() const { return (A << 24) | (R << 16) | (G << 8) | (B << 0); }
    };

    inline const FColor FColor_Gray{153};
}
