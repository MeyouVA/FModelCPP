// Ported from CUE4Parse/UE4/Objects/Core/i18N/FTextLocalizationResourceString.cs (the archive constructor).
#include "FTextLocalizationResourceString.h"

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    FTextLocalizationResourceString::FTextLocalizationResourceString(Readers::FArchive& Ar, ELocResVersion versionNumber)
    {
        String = Ar.ReadFString();
        RefCount = versionNumber >= ELocResVersion::Optimized_CRC32 ? Ar.Read<int32_t>() : -1;
    }
}
