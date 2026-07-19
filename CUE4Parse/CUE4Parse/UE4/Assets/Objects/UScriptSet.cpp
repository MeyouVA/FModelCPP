// Ported from CUE4Parse/UE4/Assets/Objects/UScriptSet.cs (element reading).
#include "UScriptSet.h"

#include "FPropertyTagData.h"
#include "../Readers/FAssetArchive.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using Readers::FAssetArchive;
    using Properties::FPropertyTagType;
    using Properties::ReadType;

    UScriptSet::UScriptSet(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType readType)
    {
        // Game-specific InnerType / InnerTypeData inference is deferred.
        if (tagData == nullptr || !tagData->InnerType.has_value())
            throw Exceptions::ParserException(Ar, "UScriptSet needs inner type");
        const std::string& innerType = *tagData->InnerType;

        if (readType != ReadType::RAW)
        {
            const int numElementsToRemove = Ar.Read<int32_t>();
            for (int i = 0; i < numElementsToRemove; i++)
                FPropertyTagType::ReadPropertyTagType(Ar, innerType, nullptr, ReadType::ARRAY);
        }

        const ReadType type = readType == ReadType::RAW ? ReadType::RAW : ReadType::ARRAY;
        const int num = Ar.Read<int32_t>();
        Properties.reserve(static_cast<size_t>(num));
        for (int i = 0; i < num; i++)
        {
            auto property = FPropertyTagType::ReadPropertyTagType(Ar, innerType, nullptr, type);
            if (property)
                Properties.push_back(std::move(property));
        }
    }

    std::string UScriptSet::ToString() const
    {
        return "[" + std::to_string(Properties.size()) + " elements]";
    }
}
