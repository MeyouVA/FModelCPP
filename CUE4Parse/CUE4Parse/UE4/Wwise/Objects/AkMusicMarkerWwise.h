// Ported from CUE4Parse/UE4/Wwise/Objects/AkMusicMarkerWwise.cs
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkMusicMarkerWwise
    {
        uint32_t Id = 0;
        double Position = 0;
        std::optional<std::string> MarkerName;

        AkMusicMarkerWwise() = default;

        explicit AkMusicMarkerWwise(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            Position = Ar.Read<double>();

            if (Ar.Version <= 62)
            {
                MarkerName = std::nullopt;
            }
            else if (Ar.Version <= 136)
            {
                // A count-prefixed ASCII blob that still carries its NUL terminator.
                auto bytes = Ar.ReadArrayCounted<uint8_t>();
                std::string name(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                while (!name.empty() && name.back() == '\0') name.pop_back();
                MarkerName = std::move(name);
            }
            else
            {
                MarkerName = Ar.ReadStzString();
            }
        }

        static std::vector<AkMusicMarkerWwise> ReadArray(FWwiseArchive& Ar)
        {
            const int count = static_cast<int>(Ar.Read<uint32_t>());
            return Ar.ReadArrayWith(count, [&Ar] { return AkMusicMarkerWwise(Ar); });
        }
    };
}
