// Ported from CUE4Parse/UE4/Objects/Core/i18N/FEntry.cs
// One translated value in a .locres: the localized string, the hash of the source string it was translated
// from (used upstream to detect stale translations), and which .locres it came out of.
#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    class FEntry : public UE4::IUStruct
    {
    public:
        // Filled in by the reader after construction — the archive constructor reads only the hash, because
        // where the string comes from depends on the format version (inline, or an index into the LUT).
        std::string LocalizedString;
        std::string LocResName;
        uint32_t SourceStringHash = 0;
        // Never set to anything but 0 by the archive constructor; InternationalizationDictionary ignores it,
        // exactly as C# does ("TODO: we ignore the value priority here").
        int32_t Priority = 0;

        explicit FEntry(Readers::FArchive& Ar);

        FEntry(std::string localizedString, std::string locResName, uint32_t sourceStringHash, int32_t priority = 0)
            : LocalizedString(std::move(localizedString)), LocResName(std::move(locResName)),
              SourceStringHash(sourceStringHash), Priority(priority) {}
    };
}
