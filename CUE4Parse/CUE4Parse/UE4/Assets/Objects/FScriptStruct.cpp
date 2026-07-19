// Ported from CUE4Parse/UE4/Assets/Objects/FScriptStruct.cs (fallback path only + ToString).
#include "FScriptStruct.h"

#include "../Readers/FAssetArchive.h"
#include "Properties/FPropertyTagType.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    FScriptStruct::FScriptStruct(Readers::FAssetArchive& Ar, const std::optional<std::string>& structName, Properties::ReadType type)
    {
        // The named-struct switch is deferred; always take the generic FStructFallback path (C# `default:` arm).
        if (type == Properties::ReadType::ZERO)
            Struct = std::make_unique<FStructFallback>();
        else
            Struct = std::make_unique<FStructFallback>(Ar, structName);
    }

    std::string FScriptStruct::ToString() const
    {
        return Struct ? Struct->ToString() + " (FStructFallback)" : "(null)";
    }
}
