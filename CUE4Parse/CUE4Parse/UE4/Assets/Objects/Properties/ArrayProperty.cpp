// Ported from CUE4Parse/UE4/Assets/Objects/Properties/ArrayProperty.cs (ctor).
#include "ArrayProperty.h"

#include "../FPropertyTagData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    ArrayProperty::ArrayProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type, int size)
        : Value(type == ReadType::ZERO
            ? UScriptArray(tagData != nullptr && tagData->InnerType.has_value() ? *tagData->InnerType : std::string("ZeroUnknown"))
            : UScriptArray(Ar, tagData, type, size))
    {
    }
}
