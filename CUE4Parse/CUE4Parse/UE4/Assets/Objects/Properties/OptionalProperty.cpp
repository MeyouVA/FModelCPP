// Ported from CUE4Parse/UE4/Assets/Objects/Properties/OptionalProperty.cs (ctor).
#include "OptionalProperty.h"

#include "../FPropertyTagData.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    namespace Exceptions = CUE4Parse::UE4::Exceptions;

    OptionalProperty::OptionalProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type)
    {
        if (tagData == nullptr)
            throw Exceptions::ParserException(Ar, "Can't load OptionalProperty without tag data");
        if (!tagData->InnerType.has_value())
            throw Exceptions::ParserException(Ar, "OptionalProperty needs inner type");

        if (type == ReadType::ZERO || !Ar.ReadBoolean())
            return; // Value stays null.

        // InnerTypeData is deferred with FPropertyTagData → nullptr.
        Value = ReadPropertyTagType(Ar, *tagData->InnerType, nullptr, ReadType::OPTIONAL);
    }
}
