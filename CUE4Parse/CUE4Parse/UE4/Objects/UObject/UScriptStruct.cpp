// Ported from CUE4Parse/UE4/Objects/UObject/UScriptStruct.cs.
#include "UScriptStruct.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Versions/EGame.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using namespace CUE4Parse::UE4::Versions;

    void UScriptStruct::Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UStruct::Deserialize(Ar, validPos);
        // Ar.Ver >= LIGHTING_CHANNEL_SUPPORT is always true for UE4+; read StructFlags.
        StructFlags = Ar.Read<EStructFlags>();
        // Ar.Game < GAME_UE4_0 (UE3) would re-read tagged properties here — not applicable for modern assets.
    }
}
