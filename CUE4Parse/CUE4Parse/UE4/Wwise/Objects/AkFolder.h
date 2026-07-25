// Ported from CUE4Parse/UE4/Wwise/Objects/AkFolder.cs
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    class AkFolder
    {
    public:
        uint32_t Offset = 0;
        uint32_t Id = 0;
        std::optional<std::string> Name;

        AkFolder() = default;

        explicit AkFolder(FWwiseArchive& Ar)
        {
            Offset = Ar.Read<uint32_t>();
            Id = Ar.Read<uint32_t>();
        }

        // The name lives elsewhere in the AKPK names section, so it is filled in as a second pass.
        // C# reads `char`, which is two bytes in .NET -- these names are UTF-16 in the container.
        void PopulateName(FWwiseArchive& Ar, int64_t namesOffset)
        {
            Ar.Position = namesOffset + Offset;
            std::string sb;
            while (true)
            {
                const uint16_t c = Ar.Read<uint16_t>();
                if (c == 0x00) break;
                sb.push_back(static_cast<char>(c));
            }

            // C#'s string.Trim() -- both ends, whitespace only.
            const auto first = sb.find_first_not_of(" \t\n\r\f\v");
            if (first == std::string::npos)
                Name = std::string();
            else
                Name = sb.substr(first, sb.find_last_not_of(" \t\n\r\f\v") - first + 1);
        }
    };
}
