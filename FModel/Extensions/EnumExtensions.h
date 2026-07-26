#pragma once
// Ported from FModel/Extensions/EnumExtensions.cs.
//
// C#'s members are generic over `Enum` and lean on reflection (GetField, GetCustomAttributes, Enum.GetValues,
// Array.IndexOf). C++ has none of that, so only the parts with a real call site are ported, and each is
// spelled as an overload on the concrete enum rather than a generic:
//
//   * GetDescription  -> description(E). For every enum that carries [Description] attributes the text is
//     already inlined next to its members (FModel/Enums.h, and `Description(E)` on the CUE4Parse side), so the
//     only member left to port here is the *fallback* branch — the one GetDescription takes when the enum has
//     no attributes at all. EGame is the enum that hits it: the settings UI renders the whole version list
//     through EnumToStringConverter, and EGame.cs declares no [Description] anywhere.
//   * ToEnum<T>, GetIndex, Next<T>, Previous<T> — no call site in the ported tree yet, and each needs
//     Enum.GetValues/Enum.TryParse on an arbitrary enum. Left out until something needs them.
//   * HasAnyFlags<T> — the flag enums that need it define their own operators (see EBulkType in Enums.h and
//     EUnluacFlags), so a generic version would have nothing to add.

#include <QString>

#include "UE4/Versions/EGame.h"

namespace FModel::Extensions
{
    // C#'s EnumExtensions.GetDescription(EGame). With no [Description] to read it runs the fallback:
    //
    //     var suffix = $"{value:D}";                       // the numeric value, as text
    //     var current = Convert.ToInt32(suffix);
    //     var mask = value.GetType() == typeof(EGame) ? ~0xFFFF : ~0xF;
    //     var target = current & mask;
    //     if (current != target)                           // i.e. this is a game, not a base engine version
    //     {
    //         var values = Enum.GetValues(value.GetType());
    //         var index = Array.IndexOf(values, value);
    //         suffix = values.GetValue(index - (current - target))?.ToString();
    //     }
    //     return $"{value} ({suffix})";
    //
    // So a game renders as "GAME_ArkSurvivalEvolved (GAME_UE4_5)" and a base version as its own name plus its
    // decimal value, "GAME_UE4_5 (67436544)". Stepping back `current - target` *positions* (not values) only
    // lands on the base version because the members between two base versions are consecutive — upstream's
    // assumption, kept here rather than replaced by a mask of the value, so the two agree even where it
    // doesn't hold.
    inline QString description(CUE4Parse::UE4::Versions::EGame value)
    {
        using namespace CUE4Parse::UE4::Versions;

        const char* name = EGameName(value);
        const QString rendered = name != nullptr ? QString::fromLatin1(name)
                                                 : QString::number(static_cast<uint32_t>(value));

        const int current = static_cast<int>(static_cast<uint32_t>(value));
        const int target = current & ~0xFFFF;
        QString suffix = QString::number(current);

        if (current != target)
        {
            size_t count = 0;
            const EGame* values = EGameValues(count);

            size_t index = count;
            for (size_t i = 0; i < count; ++i)
            {
                if (values[i] == value)
                {
                    index = i;
                    break;
                }
            }

            // C#'s Array.IndexOf returns -1 for a value that is not a member, and the indexer then throws;
            // here an unknown value simply keeps its numeric suffix. The step can also run off the front of
            // the array (C#'s `?.ToString()` would yield null and render "GAME_X ()"), which is the empty
            // suffix below.
            const size_t steps = static_cast<size_t>(current - target);
            if (index != count)
                suffix = index >= steps ? QString::fromLatin1(EGameName(values[index - steps])) : QString();
        }

        return QStringLiteral("%1 (%2)").arg(rendered, suffix);
    }
}
