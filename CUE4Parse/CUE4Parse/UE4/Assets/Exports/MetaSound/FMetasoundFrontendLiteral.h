// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendLiteral.cs
// [StructFallback]. A default value for a vertex: the type tag says which of the As* arrays is meaningful.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    enum class EMetasoundFrontendLiteralType : uint8_t
    {
        None, //< A value of None expresses that an object being constructed with a literal should be default constructed.
        Boolean,
        Integer,
        Float,
        String,
        UObject,

        NoneArray, //< A NoneArray expresses the number of objects to be default constructed.
        BooleanArray,
        IntegerArray,
        FloatArray,
        StringArray,
        UObjectArray,

        Invalid
    };

    class FMetasoundFrontendLiteral
    {
    public:
        EMetasoundFrontendLiteralType Type = EMetasoundFrontendLiteralType::None;
        int32_t AsNumDefault = 0;
        std::vector<bool> AsBoolean;
        std::vector<int32_t> AsInteger;
        std::vector<float> AsFloat;
        std::vector<std::string> AsString;
        std::vector<FPackageIndex> AsUObject;

        FMetasoundFrontendLiteral() = default;

        explicit FMetasoundFrontendLiteral(const FStructFallback& fallback)
        {
            Type = PropertyUtil::GetOrDefault<EMetasoundFrontendLiteralType>(fallback, "Type");
            AsNumDefault = PropertyUtil::GetOrDefault<int32_t>(fallback, "AsNumDefault");
            AsBoolean = PropertyUtil::GetArray<bool>(fallback, "AsBoolean");
            AsInteger = PropertyUtil::GetArray<int32_t>(fallback, "AsInteger");
            AsFloat = PropertyUtil::GetArray<float>(fallback, "AsFloat");
            AsString = PropertyUtil::GetArray<std::string>(fallback, "AsString");
            AsUObject = PropertyUtil::GetArray<FPackageIndex>(fallback, "AsUObject");
        }
    };
}
