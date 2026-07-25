// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/AbstractHierarchy.cs
#pragma once

#include <cstdint>

#include "../ICAkIndexable.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    // CAkIndexable
    // C# also declares an abstract WriteJson here; the port has no serializer layer, so the class is
    // abstract only by virtue of never being constructed directly.
    class AbstractHierarchy : public ICAkIndexable
    {
    public:
        uint32_t Id = 0;

        ~AbstractHierarchy() override = default;
    };
}
