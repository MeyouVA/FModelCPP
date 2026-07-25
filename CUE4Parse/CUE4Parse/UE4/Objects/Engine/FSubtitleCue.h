// Ported from CUE4Parse/UE4/Objects/Engine/FSubtitleCue.cs
// One line of a sound wave's subtitle track.
//
// Deliberate difference from C#: the port keeps the cue's resolved text rather than the FText itself. This
// port's FText owns a unique_ptr<FTextHistory> and so is move-only, and the property bag it is read out of
// is const; C# copies a reference to the same FText object instead.
#pragma once

#include <string>

#include "../../Assets/Exports/PropertyUtil.h"
#include "../Core/i18N/FText.h"

namespace CUE4Parse::UE4::Objects::Engine
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::Core::i18N::FText;
    namespace PropertyUtil = CUE4Parse::UE4::Assets::Exports::PropertyUtil;

    class FSubtitleCue
    {
    public:
        std::string Text;
        float Time = 0.0f;

        FSubtitleCue() = default;

        explicit FSubtitleCue(const FStructFallback& fallback)
        {
            const FText* text = nullptr;
            if (PropertyUtil::TryGet(fallback, "Text", text) && text != nullptr) Text = text->Text();
            Time = PropertyUtil::GetOrDefault<float>(fallback, "Time");
        }
    };
}
