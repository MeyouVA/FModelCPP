// Ported from CUE4Parse/UE4/Objects/Core/Math/TRangeBound.cs — one end of a TRange.
#pragma once

#include <cstdint>
#include <string>

#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    /**
     * Enumerates the valid types of range bounds.
     */
    enum class ERangeBoundTypes : uint8_t
    {
        /** The range excludes the bound. */
        Exclusive,

        /** The range includes the bound. */
        Inclusive,

        /** The bound is open. */
        Open
    };

    /**
     * Template for range bounds. Packed, so it maps onto the serialized layout as-is.
     */
#pragma pack(push, 1)
    template <typename T>
    struct TRangeBound : public UE4::IUStruct
    {
        /** Holds the type of the bound. */
        ERangeBoundTypes Type = ERangeBoundTypes::Exclusive;

        /** Holds the bound's value. */
        T Value{};

        TRangeBound() = default;
        TRangeBound(ERangeBoundTypes type, T value) : Type(type), Value(value) {}

        std::string ToString() const { return std::to_string(Value); }
    };
#pragma pack(pop)
}
