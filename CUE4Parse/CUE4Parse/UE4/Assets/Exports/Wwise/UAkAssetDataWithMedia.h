// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAssetDataWithMedia.cs
// A UAkAssetData that also owns media; the distinction is the class name only.
#pragma once

#include "UAkAssetData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    class UAkAssetDataWithMedia : public UAkAssetData
    {
    };
}
