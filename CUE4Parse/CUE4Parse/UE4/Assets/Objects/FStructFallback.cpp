// Ported from CUE4Parse/UE4/Assets/Objects/FStructFallback.cs (tagged ctor + ToString).
#include "FStructFallback.h"

#include "../Exports/UObject.h"
#include "../Readers/FAssetArchive.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    FStructFallback::FStructFallback(Readers::FAssetArchive& Ar, const std::optional<std::string>& /*structType*/)
    {
        if (Ar.HasUnversionedProperties())
            throw Exceptions::ParserException(Ar, "Unversioned struct fallback is not yet supported");

        Exports::UObject::DeserializePropertiesTagged(Properties, Ar, true);
    }

    std::string FStructFallback::ToString() const
    {
        return "[" + std::to_string(Properties.size()) + " properties]";
    }
}
