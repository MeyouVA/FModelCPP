// Ported from CUE4Parse/UE4/Wwise/Plugins/CMicrosoftHRTFSinkParams.cs
#pragma once

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    class CMicrosoftHRTFSinkParams : public IAkPluginParam
    {
    public:
        float RoomSize;

        explicit CMicrosoftHRTFSinkParams(FWwiseArchive& Ar) : RoomSize(Ar.Read<float>()) {}
    };
}
