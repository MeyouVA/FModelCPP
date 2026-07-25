// Ported from CUE4Parse/UE4/Assets/Objects/FIntBulkData.cs
#pragma once

#include <cstdint>

#include "TBulkData.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    class FIntBulkData final : public TBulkData<int32_t>
    {
    public:
        explicit FIntBulkData(FAssetArchive& Ar) : TBulkData<int32_t>(Ar) {}
    };
}
