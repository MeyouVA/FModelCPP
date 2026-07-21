// Ported from CUE4Parse/UE4/Objects/UObject/UFunction.cs
// A UFunction export = a UStruct plus function flags and (modern) event-graph fast-call info. Registered as "Function".
#pragma once

#include <cstdint>
#include <optional>

#include "UStruct.h"
#include "EFunctionFlags.h"
#include "ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class UFunction : public UStruct
    {
    public:
        EFunctionFlags FunctionFlags = FUNC_None;
        std::optional<FPackageIndex> EventGraphFunction; // UFunction
        int32_t EventGraphCallOffset = 0;

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
