// Ported from CUE4Parse/UE4/Objects/Core/i18N/FText.cs (all reading ctors).
// See FText.h for the deliberate differences (provider localization for Base only, assume-modern custom
// versions, omitted game-specific quirks).
#include "FText.h"

#include "../../../Assets/Readers/FAssetArchive.h"
#include "../../../Assets/Exports/Internationalization/UStringTable.h"
#include "../../../Exceptions/ParserException.h"
#include "../../../Versions/ObjectVersion.h"
#include "../../../../FileProvider/IFileProvider.h"

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    using namespace CUE4Parse::UE4::Versions;
    namespace Exceptions = CUE4Parse::UE4::Exceptions;

    // ---- FFormatArgumentValue ----
    FFormatArgumentValue::FFormatArgumentValue(FFormatArgumentValue&&) noexcept = default;
    FFormatArgumentValue& FFormatArgumentValue::operator=(FFormatArgumentValue&&) noexcept = default;
    FFormatArgumentValue::~FFormatArgumentValue() = default;

    FFormatArgumentValue::FFormatArgumentValue(FAssetArchive& Ar, bool /*isArgumentData*/)
    {
        // Assume-modern: FEditorObjectVersion <= TextFormatArgumentDataIsVariant is false on modern assets,
        // so the type byte is always present.
        Type = Ar.Read<EFormatArgumentType>();
        switch (Type)
        {
            case EFormatArgumentType::Text:
                TextValue = std::make_unique<FText>(Ar);
                break;
            case EFormatArgumentType::Int:
                // Assume-modern: the argument-data 32-bit legacy path is gated off, so read 64-bit.
                IntValue = Ar.Read<int64_t>();
                break;
            case EFormatArgumentType::UInt:
                UIntValue = Ar.Read<uint64_t>();
                break;
            case EFormatArgumentType::Double:
                DoubleValue = Ar.Read<double>();
                break;
            case EFormatArgumentType::Float:
                FloatValue = Ar.Read<float>();
                break;
            default:
                throw Exceptions::ParserException(Ar, "FormatArgument type not supported yet");
        }
    }

    std::string FFormatArgumentValue::ToString() const
    {
        switch (Type)
        {
            case EFormatArgumentType::Text:   return TextValue ? TextValue->Text() : std::string();
            case EFormatArgumentType::Int:    return std::to_string(IntValue);
            case EFormatArgumentType::UInt:   return std::to_string(UIntValue);
            case EFormatArgumentType::Double: return std::to_string(DoubleValue);
            case EFormatArgumentType::Float:  return std::to_string(FloatValue);
            default:                          return std::string();
        }
    }

    // ---- FNumberFormattingOptions ----
    FNumberFormattingOptions::FNumberFormattingOptions(FAssetArchive& Ar)
    {
        // Assume-modern: FEditorObjectVersion > AddedAlwaysSignNumberFormattingOption, so read the flag.
        AlwaysSign = Ar.ReadBoolean();
        UseGrouping = Ar.ReadBoolean();
        RoundingMode = Ar.Read<ERoundingMode>();
        MinimumIntegralDigits = Ar.Read<int32_t>();
        MaximumIntegralDigits = Ar.Read<int32_t>();
        MinimumFractionalDigits = Ar.Read<int32_t>();
        MaximumFractionalDigits = Ar.Read<int32_t>();
    }

    // ---- FFormatArgumentData ----
    FFormatArgumentData::FFormatArgumentData(FAssetArchive& Ar)
    {
        ArgumentName = Ar.Ver() >= EUnrealEngineObjectUE4Version::K2NODE_VAR_REFERENCEGUIDS
                           ? Ar.ReadFString()
                           : FText(Ar).Text();
        ArgumentValue = FFormatArgumentValue(Ar, true);
    }

    // ---- FTextHistory subclasses ----
    FTextHistory::None::None(FAssetArchive& Ar)
    {
        // Assume-modern: FEditorObjectVersion >= CultureInvariantTextSerializationKeyStability.
        if (Ar.ReadBoolean()) // bHasCultureInvariantString
            CultureInvariantString = Ar.ReadFString();
    }

    FTextHistory::Base::Base(FAssetArchive& Ar)
    {
        Namespace = Ar.ReadFString();
        Key = Ar.ReadFString();
        SourceString = Ar.ReadFString();
        // Assume-modern: FFortniteMainBranchObjectVersion >= AddDevNotesToFText → a dev-notes FString follows
        // for editor (non-filtered) packages.
        if (!Ar.IsFilterEditorOnly())
            Ar.SkipFString();
        // (HonorofKingsWorld/EmbersOfTheUncrowned namespace/decryption quirks omitted — see header.)
        // C#: Ar.Owner?.Provider?.Internationalization.SafeGet(ns, Key, SourceString) ?? "".
        if (Ar.Owner != nullptr)
            if (auto* provider = Ar.Owner->GetProvider())
                LocalizedString = provider->GetInternationalization().SafeGet(Namespace, Key, SourceString);
    }

    FTextHistory::Base::Base(std::string ns, std::string key, std::string sourceString, std::string localizedString)
        : Namespace(std::move(ns)), Key(std::move(key)), SourceString(std::move(sourceString)),
          LocalizedString(localizedString.empty() ? SourceString : std::move(localizedString))
    {
    }

    FTextHistory::NamedFormat::NamedFormat(FAssetArchive& Ar)
        : SourceFmt(Ar)
    {
        const int argCount = Ar.Read<int32_t>();
        Arguments.reserve(static_cast<size_t>(argCount > 0 ? argCount : 0));
        for (int i = 0; i < argCount; i++)
        {
            std::string key = Ar.ReadFString();
            Arguments.emplace_back(std::move(key), FFormatArgumentValue(Ar));
        }
    }

    FTextHistory::OrderedFormat::OrderedFormat(FAssetArchive& Ar)
        : SourceFmt(Ar)
    {
        Arguments = Ar.ReadArrayWith([&Ar] { return FFormatArgumentValue(Ar); });
    }

    FTextHistory::ArgumentFormat::ArgumentFormat(FAssetArchive& Ar)
        : SourceFmt(Ar)
    {
        Arguments = Ar.ReadArrayWith([&Ar] { return FFormatArgumentData(Ar); });
    }

    FTextHistory::FormatNumber::FormatNumber(FAssetArchive& Ar, ETextHistoryType historyType)
    {
        // SourceValue is default-constructed; the reads below preserve the on-disk order (CurrencyCode,
        // SourceValue, FormatOptions, TargetCulture).
        if (historyType == ETextHistoryType::AsCurrency &&
            Ar.Ver() >= EUnrealEngineObjectUE4Version::ADDED_CURRENCY_CODE_TO_FTEXT)
        {
            CurrencyCode = Ar.ReadFString();
        }

        SourceValue = FFormatArgumentValue(Ar);
        if (Ar.ReadBoolean()) // bHasFormatOptions
            FormatOptions.emplace(Ar);

        TargetCulture = Ar.ReadFString();
    }

    FTextHistory::AsDate::AsDate(FAssetArchive& Ar)
    {
        SourceDateTime = Ar.Read<FDateTime>();
        DateStyle = Ar.Read<EDateTimeStyle>();
        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::FTEXT_HISTORY_DATE_TIMEZONE)
            TimeZone = Ar.ReadFString();
        TargetCulture = Ar.ReadFString();
    }

    FTextHistory::AsTime::AsTime(FAssetArchive& Ar)
    {
        SourceDateTime = Ar.Read<FDateTime>();
        TimeStyle = Ar.Read<EDateTimeStyle>();
        TimeZone = Ar.ReadFString();
        TargetCulture = Ar.ReadFString();
    }

    FTextHistory::AsDateTime::AsDateTime(FAssetArchive& Ar)
    {
        SourceDateTime = Ar.Read<FDateTime>();
        DateStyle = Ar.Read<EDateTimeStyle>();
        TimeStyle = Ar.Read<EDateTimeStyle>();
        TimeZone = Ar.ReadFString();
        TargetCulture = Ar.ReadFString();
    }

    FTextHistory::Transform::Transform(FAssetArchive& Ar)
        : SourceText(Ar)
    {
        TransformType = Ar.Read<ETransformType>();
    }

    FTextHistory::StringTableEntry::StringTableEntry(FAssetArchive& Ar)
    {
        TableId = Ar.ReadFName();
        Key = Ar.ReadFString();
        // C#: if the owning provider can load a UStringTable at TableId whose table has an entry for Key, take
        // that entry as SourceString and localize it through the provider's Internationalization table.
        // (DeltaForce's trailing int is a per-game quirk and omitted.)
        if (Ar.Owner != nullptr)
        {
            if (auto* provider = Ar.Owner->GetProvider())
            {
                using CUE4Parse::UE4::Assets::Exports::Internationalization::UStringTable;
                if (auto* table = provider->TryLoadPackageObject<UStringTable>(TableId.Text()))
                {
                    const auto it = table->StringTable.KeysToEntries.find(Key);
                    if (it != table->StringTable.KeysToEntries.end())
                    {
                        SourceString = it->second;
                        LocalizedString = provider->GetInternationalization().SafeGet(
                            table->StringTable.TableNamespace, Key, it->second);
                    }
                }
            }
        }
    }

    FTextHistory::TextGenerator::TextGenerator(FAssetArchive& Ar)
    {
        GeneratorTypeID = Ar.ReadFName();
        // If not None, UE reads a generator payload; the exact layout is engine-version specific and unused
        // here (C# also does nothing with it). TODO.
    }

    // ---- FText ----
    FText::FText(FText&&) noexcept = default;
    FText& FText::operator=(FText&&) noexcept = default;
    FText::~FText() = default;

    FText::FText(uint32_t flags, ETextHistoryType historyType, std::unique_ptr<FTextHistory> textHistory)
        : Flags(static_cast<ETextFlag>(flags)), HistoryType(historyType), TextHistory(std::move(textHistory))
    {
    }

    FText::FText(const std::string& ns, const std::string& key, const std::string& sourceString,
                 const std::string& localizedString)
        : Flags(static_cast<ETextFlag>(0)), HistoryType(ETextHistoryType::Base),
          TextHistory(std::make_unique<FTextHistory::Base>(ns, key, sourceString, localizedString))
    {
    }

    FText::FText(const std::string& sourceString, const std::string& localizedString)
        : FText(std::string(), std::string(), sourceString, localizedString)
    {
    }

    FText::FText(FAssetArchive& Ar)
    {
        if (Ar.Ver() < EUnrealEngineObjectUE4Version::FTEXT_HISTORY)
        {
            const std::string sourceStringToImplant = Ar.ReadFString();
            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::ADDED_NAMESPACE_AND_KEY_DATA_TO_FTEXT)
            {
                const std::string ns = Ar.ReadFString();
                const std::string key = Ar.ReadFString();
                TextHistory = std::make_unique<FTextHistory::Base>(ns, key, sourceStringToImplant);
            }
        }

        Flags = Ar.Read<ETextFlag>();

        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::FTEXT_HISTORY)
        {
            HistoryType = Ar.Read<ETextHistoryType>();
            switch (HistoryType)
            {
                case ETextHistoryType::Base:             TextHistory = std::make_unique<FTextHistory::Base>(Ar); break;
                case ETextHistoryType::NamedFormat:      TextHistory = std::make_unique<FTextHistory::NamedFormat>(Ar); break;
                case ETextHistoryType::OrderedFormat:    TextHistory = std::make_unique<FTextHistory::OrderedFormat>(Ar); break;
                case ETextHistoryType::ArgumentFormat:   TextHistory = std::make_unique<FTextHistory::ArgumentFormat>(Ar); break;
                case ETextHistoryType::AsNumber:         TextHistory = std::make_unique<FTextHistory::FormatNumber>(Ar, HistoryType); break;
                case ETextHistoryType::AsPercent:        TextHistory = std::make_unique<FTextHistory::FormatNumber>(Ar, HistoryType); break;
                case ETextHistoryType::AsCurrency:       TextHistory = std::make_unique<FTextHistory::FormatNumber>(Ar, HistoryType); break;
                case ETextHistoryType::AsDate:           TextHistory = std::make_unique<FTextHistory::AsDate>(Ar); break;
                case ETextHistoryType::AsTime:           TextHistory = std::make_unique<FTextHistory::AsTime>(Ar); break;
                case ETextHistoryType::AsDateTime:       TextHistory = std::make_unique<FTextHistory::AsDateTime>(Ar); break;
                case ETextHistoryType::Transform:        TextHistory = std::make_unique<FTextHistory::Transform>(Ar); break;
                case ETextHistoryType::StringTableEntry: TextHistory = std::make_unique<FTextHistory::StringTableEntry>(Ar); break;
                case ETextHistoryType::TextGenerator:    TextHistory = std::make_unique<FTextHistory::TextGenerator>(Ar); break;
                default:                                 TextHistory = std::make_unique<FTextHistory::None>(Ar); break;
            }
        }
    }

    std::string FText::Text() const { return TextHistory ? TextHistory->Text() : std::string(); }
}
