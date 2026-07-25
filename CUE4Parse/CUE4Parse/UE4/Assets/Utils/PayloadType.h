// Ported from CUE4Parse/UE4/Assets/Utils/PayloadType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Utils
{
    enum class PayloadType
    {
        UBULK,
        UPTNL,
        MUBULK,
    };

    // C#'s `type.ToString().ToLowerInvariant()`, used to build the cooked-index sidecar extension in
    // TBulkData::TryGetBulkPayload. Note MUBULK's real extension there is ".m.ubulk", which that call site
    // spells out separately for the default-cooked-index case, exactly as C# does.
    constexpr const char* ToExtension(PayloadType type)
    {
        switch (type)
        {
            case PayloadType::UBULK:  return "ubulk";
            case PayloadType::UPTNL:  return "uptnl";
            case PayloadType::MUBULK: return "mubulk";
        }
        return "";
    }
}
