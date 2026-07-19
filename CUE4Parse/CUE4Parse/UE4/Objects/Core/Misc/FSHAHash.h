// Ported from CUE4Parse/UE4/Objects/Core/Misc/FSHAHash.cs
// A 20-byte SHA-1 hash. C#'s InlineArray Bytes20 becomes a std::array<uint8_t, 20>.
// (The FIoChunkHash conversion is deferred until the IO layer is ported.)
#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

#include "../../../IUStruct.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Misc
{
    struct FSHAHash : public UE4::IUStruct
    {
        static constexpr int SIZE = 20;
        std::array<uint8_t, SIZE> Hash{};

        FSHAHash() = default;

        explicit FSHAHash(Readers::FArchive& Ar)
        {
            Ar.Serialize(Hash.data(), SIZE);
        }

        FSHAHash(Readers::FArchive& Ar, int customSize)
        {
            if (customSize <= SIZE)
            {
                Ar.Serialize(Hash.data(), customSize);
            }
            else
            {
                Ar.Serialize(Hash.data(), SIZE);
                Ar.Position += customSize - SIZE;
            }
        }

        std::string ToString() const
        {
            static const char* hex = "0123456789ABCDEF";
            std::string out;
            out.reserve(SIZE * 2);
            for (uint8_t b : Hash)
            {
                out.push_back(hex[b >> 4]);
                out.push_back(hex[b & 0x0F]);
            }
            return out;
        }

        bool IsValid() const
        {
            for (uint8_t b : Hash)
                if (b != 0) return true;
            return false;
        }

        bool Equals(const FSHAHash& other) const { return Hash == other.Hash; }
        bool operator==(const FSHAHash& other) const { return Hash == other.Hash; }
        bool operator!=(const FSHAHash& other) const { return Hash != other.Hash; }
    };
}
