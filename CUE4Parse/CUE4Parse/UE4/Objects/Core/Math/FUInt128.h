// Ported from CUE4Parse/UE4/Objects/Core/Math/FUInt128.cs
// A minimal 128-bit unsigned integer, used only by FGuid's Base36 string format. C# models it as a
// reference type (class); here it is a plain value type with the same fields and methods.
#pragma once

#include <cstdint>
#include <string>

#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    class FUInt128 : public IUStruct
    {
    public:
        // Internal values representing this number.
        uint64_t Hi;
        uint64_t Lo;

        FUInt128() : Hi(0), Lo(0) {}
        explicit FUInt128(uint64_t a) : Hi(0), Lo(a) {}
        FUInt128(uint64_t a, uint64_t b) : Hi(a), Lo(b) {}

        FUInt128(uint32_t A, uint32_t B, uint32_t C, uint32_t D)
        {
            Hi = (static_cast<uint64_t>(A) << 32) | B;
            Lo = (static_cast<uint64_t>(C) << 32) | D;
        }

        void SetQuadPart(uint32_t part, uint32_t value)
        {
            switch (part)
            {
                case 3: Hi &= 4294967295u | (static_cast<uint64_t>(value) << 32); break;
                case 2: Hi &= 18446744069414584320u | value; break;
                case 1: Lo &= 4294967295u | (static_cast<uint64_t>(value) << 32); break;
                case 0: Lo &= 18446744069414584320u | value; break;
                default: break;
            }
        }

        uint32_t GetQuadPart(uint32_t part) const
        {
            switch (part)
            {
                case 3: return static_cast<uint32_t>(Hi >> 32);
                case 2: return static_cast<uint32_t>(Hi);
                case 1: return static_cast<uint32_t>(Lo >> 32);
                case 0: return static_cast<uint32_t>(Lo);
                default: break;
            }
            return 0;
        }

        uint32_t DivideInternal(uint32_t dividend, uint32_t divisor, uint32_t& remainder) const
        {
            uint64_t value = (static_cast<uint64_t>(remainder) << 32) | dividend;
            remainder = static_cast<uint32_t>(value % divisor);
            return static_cast<uint32_t>(value / divisor);
        }

        bool IsGreater(const FUInt128& other) const
        {
            if (Hi == other.Hi) return Lo > other.Lo;
            return Hi > other.Hi;
        }

        bool IsLess(const FUInt128& other) const
        {
            if (Hi == other.Hi) return Lo < other.Lo;
            return Hi < other.Hi;
        }

        FUInt128& Divide(uint32_t divisor, uint32_t& remainder)
        {
            remainder = 0;
            SetQuadPart(3, DivideInternal(GetQuadPart(3), divisor, remainder));
            SetQuadPart(2, DivideInternal(GetQuadPart(2), divisor, remainder));
            SetQuadPart(1, DivideInternal(GetQuadPart(1), divisor, remainder));
            SetQuadPart(0, DivideInternal(GetQuadPart(0), divisor, remainder));
            return *this;
        }

        std::string ToString() const
        {
            return "Hi=" + std::to_string(Hi) + " Lo=" + std::to_string(Lo);
        }
    };
}
