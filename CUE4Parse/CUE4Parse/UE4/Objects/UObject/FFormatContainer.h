// Ported from CUE4Parse/UE4/Objects/UObject/FFormatContainer.cs
// The cooked audio formats of a sound wave: one bulk-data blob per format name ("OGG", "ADPCM", ...).
//
// Deliberate difference from C#: FName has no operator<, only CompareTo (see FName.h), so the SortedDictionary
// is a std::map with an explicit comparator over the same case-insensitive ordering CompareTo defines.
#pragma once

#include <map>

#include "FName.h"
#include "../../Assets/Objects/FByteBulkData.h"
#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class FFormatContainer
    {
    public:
        struct FNameLess
        {
            bool operator()(const FName& a, const FName& b) const { return a.CompareTo(b) < 0; }
        };

        std::map<FName, FByteBulkData, FNameLess> Formats;

        FFormatContainer() = default;

        explicit FFormatContainer(FAssetArchive& Ar)
        {
            const int32_t numFormats = Ar.Read<int32_t>();
            for (int32_t i = 0; i < numFormats; i++)
            {
                // The name must be read before the value: `Formats[Ar.ReadFName()] = new FByteBulkData(Ar)`
                // has a defined left-to-right order in C#, but not as one statement here.
                FName name = Ar.ReadFName();
                Formats.emplace(std::move(name), FByteBulkData(Ar));
            }
        }
    };
}
