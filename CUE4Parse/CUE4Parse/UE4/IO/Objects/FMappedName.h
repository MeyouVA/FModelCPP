// Ported from CUE4Parse/UE4/IO/Objects/FMappedName.cs
// A packed (index, type) pair used by the IO Store name pool: the low 30 bits are the name index,
// the top 2 bits select which pool (package/container/global) the name lives in.
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::IO::Objects
{
    // [StructLayout(LayoutKind.Sequential)] — trivially copyable POD (8 bytes), read via Read<FMappedName>().
    struct FMappedName
    {
        enum class EType
        {
            Package,
            Container,
            Global
        };

        static constexpr int IndexBits = 30;
        static constexpr uint32_t IndexMask = (1u << IndexBits) - 1u;
        static constexpr uint32_t TypeMask = ~IndexMask;
        static constexpr int TypeShift = IndexBits;

        uint32_t _nameIndex = 0;
        uint32_t ExtraIndex = 0;

        uint32_t NameIndex() const { return _nameIndex & IndexMask; }
        EType Type() const { return static_cast<EType>((_nameIndex & TypeMask) >> TypeShift); }
        bool IsGlobal() const { return ((_nameIndex & TypeMask) >> TypeShift) != 0; }
    };
}
