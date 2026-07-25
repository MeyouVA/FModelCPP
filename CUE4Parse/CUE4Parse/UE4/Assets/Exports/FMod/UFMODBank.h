// Ported from CUE4Parse/UE4/Assets/Exports/FMod/UFMODBank.cs
// A marker export type: FMOD's UE plugin assets carry everything in tagged properties, so the class exists
// only so the object registry can name them. C# declares each in its own file; the port follows.
#pragma once

#include "../UObject.h"

namespace CUE4Parse::UE4::Assets::Exports::FMod
{
    class UFMODBank : public UObject
    {
    };
}
