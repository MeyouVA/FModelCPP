// Port-only helper: the three FWwiseLocalized*CookedData types in C# all build the same shape --
// Dictionary<FWwiseLanguageCookedData, TValue?> read out of a UScriptMap whose keys and values are struct
// properties. C# gets that from `kv.Key.GetValue<T>()`, which is reflection; this is the one place the
// port spells the walk out, so the three call sites stay one line each as they are in C#.
#pragma once

#include <map>
#include <optional>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Objects/UScriptMap.h"
#include "../../Objects/Properties/MapProperty.h"
#include "FWwiseLanguageCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::UScriptMap;

    // C#'s value type is nullable (`TValue?`), so a map entry with a non-struct value stays empty rather
    // than being dropped -- the language key is still meaningful on its own.
    template <typename TValue, typename Holder>
    std::map<FWwiseLanguageCookedData, std::optional<TValue>> ReadLanguageMap(
        const Holder& holder, const std::string& propertyName)
    {
        std::map<FWwiseLanguageCookedData, std::optional<TValue>> result;

        const FPropertyTag* tag = PropertyUtil::FindTag(holder.Properties, propertyName);
        if (tag == nullptr || tag->Tag == nullptr) return result;
        const auto* mapProp = dynamic_cast<const Objects::Properties::MapProperty*>(tag->Tag.get());
        if (mapProp == nullptr) return result;

        for (const auto& kv : mapProp->Value.Properties)
        {
            if (kv.first == nullptr) continue;
            const FStructFallback* keyFallback = nullptr;
            if (!PropertyUtil::PropertyValue(*kv.first, keyFallback) || keyFallback == nullptr) continue;

            std::optional<TValue> value;
            const FStructFallback* valueFallback = nullptr;
            if (kv.second != nullptr && PropertyUtil::PropertyValue(*kv.second, valueFallback) && valueFallback != nullptr)
                value.emplace(*valueFallback);

            result.emplace(FWwiseLanguageCookedData(*keyFallback), std::move(value));
        }

        return result;
    }
}
