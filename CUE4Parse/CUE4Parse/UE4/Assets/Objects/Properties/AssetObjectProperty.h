// Ported from CUE4Parse/UE4/Assets/Objects/Properties/AssetObjectProperty.cs
// An asset/class object reference serialized as a plain FString path.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class AssetObjectProperty : public TPropertyTagType<std::string>
    {
    public:
        AssetObjectProperty() { Value = std::string(); }
        explicit AssetObjectProperty(std::string value) { Value = std::move(value); }
        AssetObjectProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? std::string() : Ar.ReadFString(); }
        const char* TypeName() const override { return "AssetObjectProperty"; }
    };
}
