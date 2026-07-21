// Ported from CUE4Parse/UE4/Objects/UObject/FField.cs (base FField only; Construct/SerializeSingleField
// are defined in UnrealType.cpp where the concrete FProperty subclasses are visible).
#include "FField.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Versions/EGame.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using namespace CUE4Parse::UE4::Versions;

    void FField::Deserialize(FAssetArchive& Ar)
    {
        Name = Ar.ReadFName();
        // C#: read Flags unless a UE5.8+ cooked/editor-filtered package omits them.
        if (Ar.Game() < GAME_UE5_8 || !Ar.IsFilterEditorOnly())
            Flags = Ar.Read<EObjectFlags>();
    }
}
