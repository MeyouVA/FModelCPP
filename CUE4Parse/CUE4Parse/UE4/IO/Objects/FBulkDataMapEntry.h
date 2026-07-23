// Ported from CUE4Parse/UE4/IO/Objects/FBulkDataMapEntry.cs
// One entry of the UE5 bulk-data map (DATA_RESOURCES): where a bulk payload lives in the combined data.
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::UE4::IO::Objects
{
    struct FBulkDataCookedIndex
    {
        uint8_t Value = 0;

        static FBulkDataCookedIndex Default() { return FBulkDataCookedIndex{}; }
        bool IsDefault() const { return Value == 0; }

        // C#'s Value.ToString("D3") — the ".oNNN" cooked-index extension digits.
        std::string GetAsExtension() const
        {
            if (IsDefault()) return std::string();
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%03u", static_cast<unsigned>(Value));
            return buf;
        }

        std::string ToString() const { return GetAsExtension(); }
    };

#pragma pack(push, 1)
    struct FBulkDataMapEntry
    {
        static constexpr uint32_t Size = 32;

        uint64_t SerialOffset = 0;
        uint64_t DuplicateSerialOffset = 0;
        uint64_t SerialSize = 0;
        uint32_t Flags = 0;
        FBulkDataCookedIndex CookedIndex;
        int16_t _pad0 = 0;
        uint8_t _pad1 = 0;
    };
#pragma pack(pop)
    static_assert(sizeof(FBulkDataMapEntry) == FBulkDataMapEntry::Size);
}
