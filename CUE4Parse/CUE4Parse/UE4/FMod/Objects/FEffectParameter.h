// Ported from CUE4Parse/UE4/FMod/Objects/FEffectParameter.cs
// A single effect parameter: a type tag then either a float, a bool-as-float, or a raw byte buffer.
// The [JsonIgnore] on Buffer is dropped (no JSON layer).
#pragma once

#include <optional>
#include <stdexcept>
#include <vector>

#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FEffectParameter
    {
        int32_t Type = 0;
        float FloatValue = 0.0f;
        std::optional<std::vector<uint8_t>> Buffer;

        FEffectParameter() = default;
        explicit FEffectParameter(Readers::FArchive& Ar)
        {
            int32_t paramType = Ar.Read<int32_t>();
            if (paramType < 0 || paramType > 3)
                throw std::runtime_error("Invalid parameter type");

            Type = paramType;

            switch (Type)
            {
                case 0:
                case 1:
                    FloatValue = Ar.Read<float>();
                    break;
                case 2:
                    FloatValue = (Ar.Read<uint8_t>() != 0) ? 1.0f : 0.0f;
                    break;
                case 3:
                {
                    uint32_t length = FModReader::ReadX16(Ar);
                    if (length > 0)
                        Buffer = Ar.ReadBytes(static_cast<int>(length));
                    FloatValue = 0;
                    break;
                }
                default:
                    throw std::runtime_error("Unknown parameter type");
            }
        }
    };
}
