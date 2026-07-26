// Ported from CUE4Parse/UE4/Objects/Core/i18N/FEntry.cs (the archive constructor; the rest is inline).
#include "FEntry.h"

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    FEntry::FEntry(Readers::FArchive& Ar)
    {
        LocalizedString = std::string();
        LocResName = Ar.Name();
        SourceStringHash = Ar.Read<uint32_t>();
        Priority = 0;
    }
}
