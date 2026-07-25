// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseAssetLibraryCookedData.cs
// A library of packaged files read straight off the archive rather than out of the property bag: the
// count-prefixed array is followed immediately by each file's bulk data, in order.
#pragma once

#include <vector>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "FWwisePackagedFile.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    class FWwiseAssetLibraryCookedData : public FStructFallback
    {
    public:
        std::vector<FWwisePackagedFile> PackagedFiles;

        explicit FWwiseAssetLibraryCookedData(FAssetArchive& Ar)
            : FStructFallback(Ar, std::string("WwiseAssetLibraryCookedData"))
        {
            PackagedFiles = Ar.ReadArrayWith([&Ar] { return FWwisePackagedFile(Ar); });
            for (FWwisePackagedFile& packagedFile : PackagedFiles)
            {
                packagedFile.SerializeBulkData(Ar);
            }
        }
    };
}
