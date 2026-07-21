// Ported from CUE4Parse/UE4/Objects/UObject/UFunction.cs.
#include "UFunction.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using namespace CUE4Parse::UE4::Versions;

    void UFunction::Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UStruct::Deserialize(Ar, validPos);

        FunctionFlags = Ar.Read<EFunctionFlags>();

        // Replication info: a networked function serializes an (unused) rep offset.
        if (FunctionFlags & FUNC_Net)
        {
            const int16_t repOffset = Ar.Read<int16_t>();
            (void)repOffset;
        }

        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::SERIALIZE_BLUEPRINT_EVENTGRAPH_FASTCALLS_IN_UFUNCTION)
        {
            EventGraphFunction = FPackageIndex(Ar);
            EventGraphCallOffset = Ar.Read<int32_t>();
        }
    }
}
