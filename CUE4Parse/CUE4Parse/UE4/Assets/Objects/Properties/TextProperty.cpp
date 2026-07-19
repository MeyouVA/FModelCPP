// Ported from CUE4Parse/UE4/Assets/Objects/Properties/TextProperty.cs (ctor).
#include "TextProperty.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::Core::i18N::ETextHistoryType;
    using CUE4Parse::UE4::Objects::Core::i18N::FTextHistory;

    static FText MakeZeroText()
    {
        return FText(0, ETextHistoryType::None, std::make_unique<FTextHistory::None>());
    }

    TextProperty::TextProperty(FAssetArchive& Ar, ReadType type)
        : Value(type == ReadType::ZERO ? MakeZeroText() : FText(Ar))
    {
    }
}
