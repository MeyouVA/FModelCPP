#pragma once
// Ported from FModel/Extensions/AssetCategoryExtensions.cs.
//
// EAssetCategory packs a base category in its high 16 bits and a leaf index in its low 16; these helpers are
// the only place that layout is decoded.
//
// Deliberate differences from C#:
//   * Extension methods become free functions in this namespace, so `category.GetBaseCategory()` is spelled
//     `getBaseCategory(category)`.
//   * getBaseCategories() returns a QList rather than a lazy IEnumerable, and enumerates a written-out table
//     because C++ has no Enum.GetValues. The static_asserts below tie that table to the enum: adding a base
//     category without listing it here is caught by the count check.

#include <cstdint>

#include <QList>

#include "../Enums.h"

namespace FModel::Extensions::AssetCategoryExtensions
{
    constexpr uint32_t CategoryBase = 0x00010000;

    // Enums.h has to spell this value literally (see the note there); this is the guard that they agree.
    static_assert(static_cast<uint32_t>(EAssetCategory::All) == CategoryBase,
                  "EAssetCategory::All must equal CategoryBase");

    constexpr EAssetCategory getBaseCategory(EAssetCategory category)
    {
        return static_cast<EAssetCategory>(static_cast<uint32_t>(category) & 0xFFFF0000u);
    }

    constexpr bool isBaseCategory(EAssetCategory category)
    {
        return category == getBaseCategory(category);
    }

    constexpr bool isOfCategory(EAssetCategory item, EAssetCategory category)
    {
        return getBaseCategory(item) == getBaseCategory(category);
    }

    // C#'s Enum.GetValues<EAssetCategory>().Where(IsBaseCategory) — the tabs the asset explorer shows.
    inline QList<EAssetCategory> getBaseCategories()
    {
        return {EAssetCategory::All,       EAssetCategory::Blueprints, EAssetCategory::Mesh,
                EAssetCategory::Texture,   EAssetCategory::Materials,  EAssetCategory::Animation,
                EAssetCategory::Level,     EAssetCategory::Data,       EAssetCategory::Media,
                EAssetCategory::Particle,  EAssetCategory::GameSpecific};
    }

    // The last base category declared in Enums.h. Bumping the enum without extending getBaseCategories()
    // trips this.
    static_assert(static_cast<uint32_t>(EAssetCategory::GameSpecific) == CategoryBase + (10u << 16),
                  "getBaseCategories() is out of date with EAssetCategory");
}
