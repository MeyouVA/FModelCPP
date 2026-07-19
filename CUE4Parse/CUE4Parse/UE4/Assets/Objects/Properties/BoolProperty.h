// Ported from CUE4Parse/UE4/Assets/Objects/Properties/BoolProperty.cs
// The bool value lives in the tag data (not the value stream) for NORMAL/ZERO reads; for MAP/ARRAY/OPTIONAL
// it is a single flag byte. (The AoC RAW-reader special case is omitted with that game's reader.)
#pragma once

#include "FPropertyTagType.h"
#include "../FPropertyTagData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    class BoolProperty : public TPropertyTagType<bool>
    {
    public:
        explicit BoolProperty(bool value) { Value = value; }
        BoolProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type)
        {
            switch (type)
            {
                case ReadType::NORMAL:
                    Value = Ar.HasUnversionedProperties()
                        ? Ar.ReadFlag()
                        : (tagData != nullptr && tagData->Bool.value_or(false));
                    break;
                case ReadType::MAP:
                case ReadType::ARRAY:
                case ReadType::OPTIONAL:
                    Value = Ar.ReadFlag();
                    break;
                case ReadType::ZERO:
                    Value = tagData != nullptr && tagData->Bool.value_or(false);
                    break;
                case ReadType::RAW:
                    Value = Ar.ReadBoolean();
                    break;
            }
        }
        const char* TypeName() const override { return "BoolProperty"; }
    };
}
