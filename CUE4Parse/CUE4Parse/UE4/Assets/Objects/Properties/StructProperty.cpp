// Ported from CUE4Parse/UE4/Assets/Objects/Properties/StructProperty.cs (ctor + ToString).
#include "StructProperty.h"

#include "../FPropertyTagData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    StructProperty::StructProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type)
        : Value(Ar, tagData != nullptr ? tagData->StructType : std::nullopt,
                tagData != nullptr ? tagData->Struct : nullptr, type)
    {
    }

    std::string StructProperty::ToString() const
    {
        if (!Value.StructType)
            return "(null struct)";

        // Mirror C#: Value.ToString() ends with ")"; splice ", StructProperty" before that closing paren.
        std::string s = Value.ToString();
        const auto pos = s.rfind(')');
        if (pos != std::string::npos)
            s = s.substr(0, pos);
        return s + ", StructProperty)";
    }
}
