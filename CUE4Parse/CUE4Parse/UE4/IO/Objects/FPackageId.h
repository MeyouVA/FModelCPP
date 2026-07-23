// Ported from CUE4Parse/UE4/IO/Objects/FPackageId.cs
// The CityHash64-of-lowercased-UTF16-name id that keys packages in IO Store containers.
//
// Deliberate difference from C#: FromName widens each byte of the std::string to a UTF-16LE code unit
// before hashing. That matches C# exactly for ASCII names (which UE package paths are in practice); a
// non-ASCII name would need real UTF-8 -> UTF-16 conversion. Documented rather than guessed at.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../../Utils/CityHash.h"

namespace CUE4Parse::UE4::IO::Objects
{
    struct FPackageId
    {
        uint64_t id = 0;

        FPackageId() = default;
        explicit FPackageId(uint64_t inId) : id(inId) {}

        static char ToLower(char input)
        {
            return static_cast<char>(
                static_cast<uint32_t>(input) +
                (((static_cast<uint32_t>(input) - 'A' < 26u) ? 1u : 0u) << 5));
        }

        static FPackageId FromName(const std::string& name)
        {
            // Lowercase, then widen to UTF-16LE bytes like C#'s Encoding.Unicode.GetBytes.
            std::vector<uint8_t> nameBuf(name.size() * 2, 0);
            for (size_t i = 0; i < name.size(); ++i)
                nameBuf[i * 2] = static_cast<uint8_t>(ToLower(name[i]));
            return FPackageId(Utils::CityHash::CityHash64(nameBuf.data(), static_cast<uint32_t>(nameBuf.size())));
        }

        bool operator==(const FPackageId& other) const { return id == other.id; }
        bool operator<(const FPackageId& other) const { return id < other.id; }

        std::string ToString() const { return std::to_string(id); }
    };
    static_assert(sizeof(FPackageId) == 8);
}
