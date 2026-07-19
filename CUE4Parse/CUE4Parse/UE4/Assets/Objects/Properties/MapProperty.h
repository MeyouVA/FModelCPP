// Ported from CUE4Parse/UE4/Assets/Objects/Properties/MapProperty.cs
// A MapProperty value: wraps a UScriptMap. (Derives FPropertyTagType directly — see the note on StructProperty.)
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../UScriptMap.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class MapProperty : public FPropertyTagType
    {
    public:
        UScriptMap Value;

        MapProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type);
        explicit MapProperty(UScriptMap value) : Value(std::move(value)) {}

        const char* TypeName() const override { return "MapProperty"; }
        std::string ToString() const override { return Value.ToString() + " (MapProperty)"; }
    };
}
