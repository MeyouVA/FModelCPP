// Ported from CUE4Parse/UE4/Objects/Engine/UUserDefinedEnum.cs
// A Blueprint-authored enum. Serialization is entirely UEnum's; C# only adds a (commented-out) DisplayNameMap,
// so this is a pure subclass registered under "UserDefinedEnum".
#pragma once

#include "../UObject/UEnum.h"

namespace CUE4Parse::UE4::Objects::Engine
{
    class UUserDefinedEnum : public CUE4Parse::UE4::Objects::UObject::UEnum
    {
        // C# has a commented-out `Dictionary<FName, FText> DisplayNameMap;` — not deserialized, so nothing here.
    };
}
