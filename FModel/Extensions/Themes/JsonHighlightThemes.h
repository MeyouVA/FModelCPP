#pragma once
// Ported from FModel/Extensions/Themes/JsonHighlightThemes.cs — the EJsonHighlightTheme enum only.
//
// The palettes themselves (JsonHighlightPalette / Get) are deferred: they are expressed as AvalonEdit
// HighlightColor values, and the syntax-highlighting layer is not ported yet. The enum comes first because
// UserSettings persists the selected theme.

#include <QString>

namespace FModel::Extensions::Themes
{
    enum class EJsonHighlightTheme
    {
        Default,
        MintLavender,
        SoftBlueGreen,
        PurpleCyan,
        NeutralWarm,
        Nord,
        Mocha,
        TokyoNight,
        OneDark,
        GruvboxDark,
        RosePine,
        Monokai,
        Oceanic,
        Forest,
        Amber,
        Iceberg
    };

    inline QString description(EJsonHighlightTheme value)
    {
        switch (value)
        {
            case EJsonHighlightTheme::Default:       return QStringLiteral("Default");
            case EJsonHighlightTheme::MintLavender:  return QStringLiteral("Mint Lavender");
            case EJsonHighlightTheme::SoftBlueGreen: return QStringLiteral("Soft Blue");
            case EJsonHighlightTheme::PurpleCyan:    return QStringLiteral("Purple Cyan");
            case EJsonHighlightTheme::NeutralWarm:   return QStringLiteral("Neutral Warm");
            case EJsonHighlightTheme::Nord:          return QStringLiteral("Nord");
            case EJsonHighlightTheme::Mocha:         return QStringLiteral("Mocha");
            case EJsonHighlightTheme::TokyoNight:    return QStringLiteral("Tokyo Night");
            case EJsonHighlightTheme::OneDark:       return QStringLiteral("One Dark");
            case EJsonHighlightTheme::GruvboxDark:   return QStringLiteral("Gruvbox Dark");
            case EJsonHighlightTheme::RosePine:      return QString::fromUtf8("Ros\xc3\xa9 Pine");
            case EJsonHighlightTheme::Monokai:       return QStringLiteral("Monokai");
            case EJsonHighlightTheme::Oceanic:       return QStringLiteral("Oceanic");
            case EJsonHighlightTheme::Forest:        return QStringLiteral("Forest");
            case EJsonHighlightTheme::Amber:         return QStringLiteral("Amber");
            case EJsonHighlightTheme::Iceberg:       return QStringLiteral("Iceberg");
        }
        return QString();
    }
}
