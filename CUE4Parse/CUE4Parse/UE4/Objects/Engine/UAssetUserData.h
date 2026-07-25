// Ported from CUE4Parse/UE4/Objects/Engine/UAssetUserData.cs
// A bare UObject subclass: the base every "extra data hanging off an asset" type derives from. It adds
// nothing of its own -- the payload is whatever tagged properties the concrete subclass declares.
#pragma once

#include "../../Assets/Exports/UObject.h"

namespace CUE4Parse::UE4::Objects::Engine
{
    class UAssetUserData : public Assets::Exports::UObject
    {
    };
}
