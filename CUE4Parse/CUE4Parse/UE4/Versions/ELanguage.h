// Ported from CUE4Parse/UE4/Versions/ELanguage.cs
#pragma once

namespace CUE4Parse::UE4::Versions
{
    enum class ELanguage
    {
        English,
        AustralianEnglish,
        BritishEnglish,
        French,
        German,
        Italian,
        Spanish,
        SpanishLatin,
        SpanishMexico,
        Arabic,
        Japanese,
        Korean,
        Polish,
        Portuguese,
        PortugueseBrazil,
        Russian,
        Turkish,
        Chinese,
        TraditionalChinese,
        Swedish,
        Thai,
        Indonesian,
        VietnameseVietnam,
        Zulu
    };

    // C#'s [Description] attribute, which the settings UI reads through EnumExtensions.GetDescription().
    // The port exposes it as an overload set instead: a plain function per enum, resolved by argument type.
    // Kept as const char* (UTF-8) so CUE4Parse stays free of any UI/Qt dependency.
    inline const char* Description(ELanguage value)
    {
        switch (value)
        {
            case ELanguage::English:            return "English";
            case ELanguage::AustralianEnglish:  return "Australian English";
            case ELanguage::BritishEnglish:     return "British English";
            case ELanguage::French:             return "Fran\xc3\xa7" "ais";
            case ELanguage::German:             return "Deutsch";
            case ELanguage::Italian:            return "Italiano";
            case ELanguage::Spanish:            return "Espa\xc3\xb1ol";
            case ELanguage::SpanishLatin:       return "Espa\xc3\xb1ol (Latinoam\xc3\xa9ricano)";
            case ELanguage::SpanishMexico:      return "Espa\xc3\xb1ol (Mexicano)";
            case ELanguage::Arabic:             return "\xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9 (Arabic)";
            case ELanguage::Japanese:           return "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (Japanese)";
            case ELanguage::Korean:             return "\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4 (Korean)";
            case ELanguage::Polish:             return "Polski";
            case ELanguage::Portuguese:         return "Portugu\xc3\xaas";
            case ELanguage::PortugueseBrazil:   return "Portugu\xc3\xaas (Brasil)";
            case ELanguage::Russian:            return "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9";
            case ELanguage::Turkish:            return "T\xc3\xbcrk\xc3\xa7" "e";
            case ELanguage::Chinese:            return "\xe4\xb8\xad\xe6\x96\x87 (Chinese)";
            case ELanguage::TraditionalChinese: return "\xe4\xb8\xad\xe6\x96\x87(\xe5\x8f\xb0\xe7\x81\xa3) (Traditional Chinese)";
            case ELanguage::Swedish:            return "Svenska";
            case ELanguage::Thai:               return "\xe0\xb9\x84\xe0\xb8\x97\xe0\xb8\xa2 / Phasa Thai";
            case ELanguage::Indonesian:         return "Bahasa Indonesia";
            case ELanguage::VietnameseVietnam:  return "Vi\xe1\xbb\x87tnam";
            case ELanguage::Zulu:               return "isiZulu";
        }
        return "";
    }
}
