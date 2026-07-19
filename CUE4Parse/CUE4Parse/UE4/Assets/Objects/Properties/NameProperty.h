// Ported from CUE4Parse/UE4/Assets/Objects/Properties/NameProperty.cs
#pragma once

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FName;

    class NameProperty : public TPropertyTagType<FName>
    {
    public:
        explicit NameProperty(FName value) { Value = std::move(value); }
        NameProperty(FAssetArchive& Ar, ReadType type) { Value = type == ReadType::ZERO ? FName() : Ar.ReadFName(); }
        const char* TypeName() const override { return "NameProperty"; }
    };
}
