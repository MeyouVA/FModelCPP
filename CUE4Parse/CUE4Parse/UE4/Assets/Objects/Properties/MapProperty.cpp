// Ported from CUE4Parse/UE4/Assets/Objects/Properties/MapProperty.cs (ctor).
#include "MapProperty.h"

#include "../FPropertyTagData.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    MapProperty::MapProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type)
    {
        if (type == ReadType::ZERO)
            return; // Value is an empty UScriptMap.

        if (tagData == nullptr)
            throw Exceptions::ParserException(Ar, "Can't load MapProperty without tag data");
        Value = UScriptMap(Ar, tagData, type);
    }
}
