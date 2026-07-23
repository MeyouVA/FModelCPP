// Ported from CUE4Parse/UE4/Assets/Objects/FStructFallback.cs (tagged ctor + ToString).
#include "FStructFallback.h"

#include "../Exports/UObject.h"
#include "../Readers/FAssetArchive.h"
#include "../../Exceptions/ParserException.h"
#include "../../Objects/UObject/UScriptClass.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    FStructFallback::FStructFallback(Readers::FAssetArchive& Ar, const std::optional<std::string>& structType)
    {
        if (Ar.HasUnversionedProperties())
        {
            // C#: new UScriptClass(structType) — a named stand-in resolved through the mappings table. The
            // object only lives for the duration of the read, so a stack instance suffices here.
            if (!structType.has_value())
                throw Exceptions::ParserException(Ar, "For unversioned struct fallback the struct type cannot be null");
            const CUE4Parse::UE4::Objects::UObject::UScriptClass scriptClass(*structType);
            Exports::UObject::DeserializePropertiesUnversioned(Properties, Ar, scriptClass);
            return;
        }

        Exports::UObject::DeserializePropertiesTagged(Properties, Ar, true);
    }

    FStructFallback::FStructFallback(Readers::FAssetArchive& Ar, const CUE4Parse::UE4::Objects::UObject::UStruct* structType)
    {
        if (Ar.HasUnversionedProperties())
        {
            if (structType == nullptr)
                throw Exceptions::ParserException(Ar, "For unversioned struct fallback the struct type cannot be null");
            Exports::UObject::DeserializePropertiesUnversioned(Properties, Ar, *structType);
            return;
        }

        Exports::UObject::DeserializePropertiesTagged(Properties, Ar, true);
    }

    std::string FStructFallback::ToString() const
    {
        return "[" + std::to_string(Properties.size()) + " properties]";
    }
}
