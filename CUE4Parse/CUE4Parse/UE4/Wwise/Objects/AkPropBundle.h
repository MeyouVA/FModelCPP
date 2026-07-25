// Ported from CUE4Parse/UE4/Wwise/Objects/AkPropBundle.cs
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    // C# models this as an explicit-layout union of a float and a uint over the same four bytes. There is
    // no type tag on the wire -- the subclass decides -- so C#'s Value property guesses by magnitude, and
    // that guess is carried over verbatim rather than "fixed".
    struct AkUnionValue
    {
        uint32_t u32 = 0;

        AkUnionValue() = default;
        explicit AkUnionValue(uint32_t val) : u32(val) {}

        float f32() const
        {
            float f;
            std::memcpy(&f, &u32, sizeof(f));
            return f;
        }

        // Guessing value type, in Wwise it's determined by the subclass, this is simply more convenient
        bool IsFloat() const { return u32 > 0x10000000; }

        static AkUnionValue Read(FWwiseArchive& Ar)
        {
            return AkUnionValue(Ar.Read<uint32_t>());
        }
    };

    struct AkProp
    {
        uint8_t Id = 0;
        AkUnionValue Value;

        AkProp() = default;
        AkProp(uint8_t id, AkUnionValue value) : Id(id), Value(value) {}
    };

    struct AkPropRange
    {
        uint8_t Id = 0;
        AkUnionValue Min;
        AkUnionValue Max;

        AkPropRange() = default;
        AkPropRange(uint8_t id, AkUnionValue min, AkUnionValue max) : Id(id), Min(min), Max(max) {}
    };

    struct AkPropBundle
    {
        std::vector<AkProp> Props;
        std::vector<AkPropRange> PropRanges;

        AkPropBundle() = default;

        explicit AkPropBundle(FWwiseArchive& Ar)
        {
            Props = ReadSequentialAkProp(Ar);
            PropRanges = ReadSequentialAkPropRange(Ar);
        }

        // "Sequential" because all the ids come first and all the values follow, rather than id/value pairs.
        static std::vector<AkProp> ReadSequentialAkProp(FWwiseArchive& Ar)
        {
            const int propCount = Ar.Read<uint8_t>();
            auto ids = Ar.ReadArray<uint8_t>(propCount);

            std::vector<AkProp> props;
            props.reserve(static_cast<size_t>(propCount));
            for (int i = 0; i < propCount; i++)
                props.emplace_back(ids[i], AkUnionValue::Read(Ar));

            return props;
        }

        static std::vector<AkPropRange> ReadSequentialAkPropRange(FWwiseArchive& Ar)
        {
            const int propCount = Ar.Read<uint8_t>();
            auto ids = Ar.ReadArray<uint8_t>(propCount);

            std::vector<AkPropRange> ranges;
            ranges.reserve(static_cast<size_t>(propCount));
            for (int i = 0; i < propCount; i++)
            {
                AkUnionValue min = AkUnionValue::Read(Ar);
                AkUnionValue max = AkUnionValue::Read(Ar);
                ranges.emplace_back(ids[i], min, max);
            }

            return ranges;
        }
    };
}
