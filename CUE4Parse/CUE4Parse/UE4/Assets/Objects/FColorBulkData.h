// Ported from CUE4Parse/UE4/Assets/Objects/FColorBulkData.cs
#pragma once

#include "TBulkData.h"
#include "../../Objects/Core/Math/FColor.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    class FColorBulkData final : public TBulkData<CUE4Parse::UE4::Objects::Core::Math::FColor>
    {
    public:
        explicit FColorBulkData(FAssetArchive& Ar)
            : TBulkData<CUE4Parse::UE4::Objects::Core::Math::FColor>(Ar) {}
    };
}
