// Ported from CUE4Parse/UE4/Objects/Core/i18N/FTextKey.cs (the archive constructor; the rest is inline).
#include "FTextKey.h"

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    FTextKey::FTextKey(Readers::FArchive& Ar, ELocResVersion versionNum)
    {
        StrHash = 0;
        if (versionNum >= ELocResVersion::Optimized_CRC32)
        {
            StrHash = Ar.Read<uint32_t>();
        }

        Str = Ar.ReadFString();
    }
}
