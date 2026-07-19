// Ported from CUE4Parse/UE4/Assets/Objects/Properties/IntProperty.cs
#pragma once

#include <cstdint>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class IntProperty : public TPropertyTagType<int32_t>
    {
    public:
        explicit IntProperty(int32_t value) { Value = value; }
        IntProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? 0 : Ar.Read<int32_t>(); }
        const char* TypeName() const override { return "IntProperty"; }
    };
}
