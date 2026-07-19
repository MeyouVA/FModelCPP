// Ported from CUE4Parse/UE4/Objects/UObject/ScriptDelegates.cs (reading ctors).
#include "ScriptDelegates.h"

#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    FScriptDelegate::FScriptDelegate(Assets::Readers::FAssetArchive& Ar)
        : Object(Ar), FunctionName(Ar.ReadFName())
    {
    }

    FMulticastScriptDelegate::FMulticastScriptDelegate(Assets::Readers::FAssetArchive& Ar)
    {
        InvocationList = Ar.ReadArrayWith([&Ar] { return FScriptDelegate(Ar); });
    }
}
