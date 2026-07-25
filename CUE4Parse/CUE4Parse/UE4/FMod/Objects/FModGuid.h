// Ported from CUE4Parse/UE4/FMod/Objects/FModGuid.cs
// FMOD's 16-byte GUID: four little-endian uint32s. Note this is NOT laid out like UE's FGuid -- the
// FGuid conversion swaps/rotates components to match FMOD's on-wire ordering (see the FGuid ctor).
//
// The JsonConverter attribute is dropped (the port has no JSON layer). Used as a dictionary key
// throughout the bank tree, so a std::hash specialization is provided below.
#pragma once

#include <cstdint>
#include <string>

#include "../../Objects/Core/Misc/FGuid.h"
#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FModGuid
    {
        uint32_t Data1 = 0;
        uint32_t Data2 = 0;
        uint32_t Data3 = 0;
        uint32_t Data4 = 0;

        FModGuid() = default;

        explicit FModGuid(Readers::FArchive& Ar)
        {
            Data1 = Ar.Read<uint32_t>();
            Data2 = Ar.Read<uint32_t>();
            Data3 = Ar.Read<uint32_t>();
            Data4 = Ar.Read<uint32_t>();
        }

        // Mirrors the FGuid ctor: FMOD reorders relative to UE's A/B/C/D. C# uses BinaryPrimitives
        // ReverseEndianness on C/D and a 16-bit rotate on B.
        explicit FModGuid(const CUE4Parse::UE4::Objects::Core::Misc::FGuid& fguid)
        {
            Data1 = fguid.A;
            Data2 = (fguid.B << 16) | (fguid.B >> 16);
            Data3 = ReverseEndianness(fguid.C);
            Data4 = ReverseEndianness(fguid.D);
        }

        bool IsEmpty() const { return Data1 == 0 && Data2 == 0 && Data3 == 0 && Data4 == 0; }

        bool Equals(const FModGuid& other) const
        {
            return Data1 == other.Data1 && Data2 == other.Data2 &&
                   Data3 == other.Data3 && Data4 == other.Data4;
        }

        bool operator==(const FModGuid& other) const { return Equals(other); }
        bool operator!=(const FModGuid& other) const { return !Equals(other); }

    private:
        static uint32_t ReverseEndianness(uint32_t v)
        {
            return (v >> 24) | ((v >> 8) & 0x0000FF00u) | ((v << 8) & 0x00FF0000u) | (v << 24);
        }
    };
}

namespace std
{
    template <>
    struct hash<CUE4Parse::UE4::FMod::Objects::FModGuid>
    {
        // Mirrors C#'s HashCode.Combine(Data1..Data4) closely enough for use as a bucket key.
        size_t operator()(const CUE4Parse::UE4::FMod::Objects::FModGuid& g) const noexcept
        {
            size_t h = 17;
            h = h * 31 + g.Data1;
            h = h * 31 + g.Data2;
            h = h * 31 + g.Data3;
            h = h * 31 + g.Data4;
            return h;
        }
    };
}
