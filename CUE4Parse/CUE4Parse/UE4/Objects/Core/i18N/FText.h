// Ported from CUE4Parse/UE4/Objects/Core/i18N/FText.cs
// FText (a localizable text value) and its FTextHistory hierarchy, plus the format-argument / number-formatting
// helper structs. Read from an FAssetArchive.
//
// Deliberate differences from C#:
//   * Provider localization is deferred (no IFileProvider): FTextHistory::Base and StringTableEntry read their
//     source/key fields but LocalizedString stays empty (C# yields "" without a provider too). StringTableEntry's
//     UStringTable load is skipped.
//   * Custom-version gates (FEditorObjectVersion / FFortniteMainBranchObjectVersion / FUE5ReleaseStreamObjectVersion)
//     aren't ported, so the port assumes modern assets and takes their modern outcome (documented at each site).
//   * Game-specific quirks (Splitgate2 / DeltaForce / HonorofKingsWorld / EmbersOfTheUncrowned) are omitted. TODO.
//   * FFormatArgumentValue's object Value becomes explicit typed fields keyed by Type (no C# boxing).
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../../../IUStruct.h"
#include "../Misc/FDateTime.h"
#include "../../UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    using Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::Core::Misc::FDateTime;
    using CUE4Parse::UE4::Objects::UObject::FName;

    enum class ETextFlag : uint32_t
    {
        Transient = 1 << 0,
        CultureInvariant = 1 << 1,
        ConvertedProperty = 1 << 2,
        Immutable = 1 << 3,
        InitializedFromString = 1 << 4,
    };

    enum class ETextHistoryType : int8_t
    {
        None = -1,
        Base = 0,
        NamedFormat,
        OrderedFormat,
        ArgumentFormat,
        AsNumber,
        AsPercent,
        AsCurrency,
        AsDate,
        AsTime,
        AsDateTime,
        Transform,
        StringTableEntry,
        TextGenerator,
    };

    enum class EFormatArgumentType : int8_t { Int, UInt, Float, Double, Text, Gender };

    enum class ERoundingMode : int8_t
    {
        HalfToEven, HalfFromZero, HalfToZero, FromZero, ToZero, ToNegativeInfinity, ToPositiveInfinity,
    };

    enum class EDateTimeStyle : int8_t { Default, Short, Medium, Long, Full };

    enum class ETransformType : uint8_t { ToLower = 0, ToUpper };

    // Forward declaration so the argument/history types can reference FText before it is fully defined.
    class FText;

    // A single format-argument value (int/uint/float/double or a nested FText). Move-only (owns an FText).
    class FFormatArgumentValue : public UE4::IUStruct
    {
    public:
        EFormatArgumentType Type{};
        int64_t IntValue = 0;
        uint64_t UIntValue = 0;
        double DoubleValue = 0.0;
        float FloatValue = 0.0f;
        std::unique_ptr<FText> TextValue;

        FFormatArgumentValue() = default;
        explicit FFormatArgumentValue(FAssetArchive& Ar, bool isArgumentData = false);
        FFormatArgumentValue(FFormatArgumentValue&&) noexcept;
        FFormatArgumentValue& operator=(FFormatArgumentValue&&) noexcept;
        ~FFormatArgumentValue();

        std::string ToString() const;
    };

    class FNumberFormattingOptions : public UE4::IUStruct
    {
    public:
        bool AlwaysSign = false;
        bool UseGrouping = true;
        ERoundingMode RoundingMode = ERoundingMode::HalfToEven;
        int32_t MinimumIntegralDigits = 1;
        int32_t MaximumIntegralDigits = 308 + 15 + 1;
        int32_t MinimumFractionalDigits = 0;
        int32_t MaximumFractionalDigits = 3;

        FNumberFormattingOptions() = default;
        explicit FNumberFormattingOptions(FAssetArchive& Ar);
    };

    // Abstract history record backing an FText; Text() yields the display string.
    class FTextHistory : public UE4::IUStruct
    {
    public:
        virtual ~FTextHistory() = default;
        virtual std::string Text() const = 0;

        class None;
        class Base;
        class NamedFormat;
        class OrderedFormat;
        class ArgumentFormat;
        class FormatNumber;
        class AsDate;
        class AsTime;
        class AsDateTime;
        class Transform;
        class StringTableEntry;
        class TextGenerator;
    };

    class FText : public UE4::IUStruct
    {
    public:
        ETextFlag Flags{};
        ETextHistoryType HistoryType = ETextHistoryType::None;
        std::unique_ptr<FTextHistory> TextHistory;

        explicit FText(FAssetArchive& Ar);
        FText(uint32_t flags, ETextHistoryType historyType, std::unique_ptr<FTextHistory> textHistory);
        // Convenience ctors mirroring C#'s string overloads (build a Base history).
        explicit FText(const std::string& sourceString, const std::string& localizedString = "");
        FText(const std::string& ns, const std::string& key, const std::string& sourceString,
              const std::string& localizedString);

        FText(FText&&) noexcept;
        FText& operator=(FText&&) noexcept;
        ~FText();

        std::string Text() const;
        std::string ToString() const { return Text(); }
    };

    class FFormatArgumentData : public UE4::IUStruct
    {
    public:
        std::string ArgumentName;
        FFormatArgumentValue ArgumentValue;

        explicit FFormatArgumentData(FAssetArchive& Ar);
    };

    // --- FTextHistory subclasses ---

    class FTextHistory::None : public FTextHistory
    {
    public:
        std::optional<std::string> CultureInvariantString;
        None() = default;
        explicit None(FAssetArchive& Ar);
        std::string Text() const override { return CultureInvariantString.value_or(std::string()); }
    };

    class FTextHistory::Base : public FTextHistory
    {
    public:
        std::string Namespace;
        std::string Key;
        std::string SourceString;
        std::string LocalizedString;
        explicit Base(FAssetArchive& Ar);
        Base(std::string ns, std::string key, std::string sourceString, std::string localizedString = "");
        std::string Text() const override { return LocalizedString; }
    };

    class FTextHistory::NamedFormat : public FTextHistory
    {
    public:
        FText SourceFmt;
        std::vector<std::pair<std::string, FFormatArgumentValue>> Arguments;
        explicit NamedFormat(FAssetArchive& Ar);
        std::string Text() const override { return SourceFmt.Text(); }
    };

    class FTextHistory::OrderedFormat : public FTextHistory
    {
    public:
        FText SourceFmt;
        std::vector<FFormatArgumentValue> Arguments;
        explicit OrderedFormat(FAssetArchive& Ar);
        std::string Text() const override { return SourceFmt.Text(); }
    };

    class FTextHistory::ArgumentFormat : public FTextHistory
    {
    public:
        FText SourceFmt;
        std::vector<FFormatArgumentData> Arguments;
        explicit ArgumentFormat(FAssetArchive& Ar);
        std::string Text() const override { return SourceFmt.Text(); }
    };

    class FTextHistory::FormatNumber : public FTextHistory
    {
    public:
        std::optional<std::string> CurrencyCode;
        FFormatArgumentValue SourceValue;
        std::optional<FNumberFormattingOptions> FormatOptions;
        std::string TargetCulture;
        FormatNumber(FAssetArchive& Ar, ETextHistoryType historyType);
        std::string Text() const override { return SourceValue.ToString(); }
    };

    class FTextHistory::AsDate : public FTextHistory
    {
    public:
        FDateTime SourceDateTime;
        EDateTimeStyle DateStyle{};
        std::optional<std::string> TimeZone;
        std::string TargetCulture;
        explicit AsDate(FAssetArchive& Ar);
        std::string Text() const override { return SourceDateTime.ToString(); }
    };

    class FTextHistory::AsTime : public FTextHistory
    {
    public:
        FDateTime SourceDateTime;
        EDateTimeStyle TimeStyle{};
        std::string TimeZone;
        std::string TargetCulture;
        explicit AsTime(FAssetArchive& Ar);
        std::string Text() const override { return SourceDateTime.ToString(); }
    };

    class FTextHistory::AsDateTime : public FTextHistory
    {
    public:
        FDateTime SourceDateTime;
        EDateTimeStyle DateStyle{};
        EDateTimeStyle TimeStyle{};
        std::string TimeZone;
        std::string TargetCulture;
        explicit AsDateTime(FAssetArchive& Ar);
        std::string Text() const override { return SourceDateTime.ToString(); }
    };

    class FTextHistory::Transform : public FTextHistory
    {
    public:
        FText SourceText;
        ETransformType TransformType{};
        explicit Transform(FAssetArchive& Ar);
        std::string Text() const override { return SourceText.Text(); }
    };

    class FTextHistory::StringTableEntry : public FTextHistory
    {
    public:
        FName TableId;
        std::string Key;
        std::string SourceString;
        std::string LocalizedString;
        explicit StringTableEntry(FAssetArchive& Ar);
        std::string Text() const override { return LocalizedString; }
    };

    class FTextHistory::TextGenerator : public FTextHistory
    {
    public:
        FName GeneratorTypeID;
        std::vector<uint8_t> GeneratorContents;
        explicit TextGenerator(FAssetArchive& Ar);
        std::string Text() const override { return GeneratorTypeID.Text(); }
    };
}
