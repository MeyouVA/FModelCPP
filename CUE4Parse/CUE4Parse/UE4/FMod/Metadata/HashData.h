// Ported from CUE4Parse/UE4/FMod/Metadata/HashData.cs
// A HASH chunk: an element list of FHashInfo. C#'s implicit conversion to FHashInfo[] is expressed here
// by FModReader simply taking .Hashes.
#pragma once

#include <vector>

#include "FHashInfo.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Metadata
{
    struct HashData
    {
        std::vector<FHashInfo> Hashes;

        HashData() = default;
        explicit HashData(Readers::FArchive& Ar)
        {
            Hashes = FModReader::ReadElemListImp<FHashInfo>(Ar);
        }
    };
}
