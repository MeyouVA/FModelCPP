// Ported from CUE4Parse/UE4/Objects/UObject/UScriptStruct.cs
// A UScriptStruct export = a UStruct plus StructFlags. Registered as "ScriptStruct".
#pragma once

#include <cstdint>

#include "UStruct.h"
#include "EStructFlags.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class UScriptStruct : public UStruct
    {
    public:
        EStructFlags StructFlags = STRUCT_NoFlags;

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
