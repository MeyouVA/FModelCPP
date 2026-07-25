#include "WwiseReader.h"

namespace CUE4Parse::UE4::Wwise
{
    namespace Objects
    {
        int64_t AkEntry::ReadData(FWwiseArchive& Ar, const WwiseDataSource& source)
        {
            Data = WwiseReader::ReadDeferredByteData(
                Ar, source, static_cast<int64_t>(Offset) * OffsetMultiplier, Size);
            return Data->LoadedSize();
        }
    }

    WwiseReader::WwiseReader(FWwiseArchive& Ar, const WwiseDataSource& source, int64_t size)
        : Path(Ar.Name()), _source(&source)
    {
        TotalSize = size == -1 ? Ar.Length : size;
        const int64_t end = size == -1 ? Ar.Length : Ar.Position + size;

        while (Ar.Position < end)
        {
            const auto sectionIdentifier = Ar.Read<EChunkID>();
            const int32_t sectionLength = Ar.Read<int32_t>();
            const int64_t position = Ar.Position;

            switch (sectionIdentifier)
            {
                case EChunkID::AKPK:
                {
                    const FAKPKHeader akpkHeader(Ar);
                    if (!akpkHeader.Endianness)
                        throw Exceptions::ParserException(Ar, "'" + Ar.Name() + "' has unsupported endianness.");

                    Ar.Position = FAKPKHeader::NamesOffset;
                    auto folders = Ar.ReadArrayWith([&Ar] { return AkFolder(Ar); });
                    for (AkFolder& folder : folders)
                        folder.PopulateName(Ar, FAKPKHeader::NamesOffset);

                    Ar.Position = akpkHeader.BanksOffset();
                    auto bankEntries = Ar.ReadArrayWith([&Ar] { return AkEntry(Ar, true, false); });

                    Ar.Position = akpkHeader.WemsOffset();
                    AKPKWemEntries = Ar.ReadArrayWith([&Ar] { return AkEntry(Ar, false, false); });
                    Ar.Position = akpkHeader.ExternalWemsOffset();
                    for (AkEntry& external : Ar.ReadArrayWith([&Ar] { return AkEntry(Ar, false, true); }))
                        AKPKWemEntries.push_back(external);

                    for (AkEntry& entry : bankEntries)
                    {
                        entry.ReadAudioPath(folders);
                        Ar.Position = static_cast<int64_t>(entry.Offset) * entry.OffsetMultiplier;
                        auto bank = std::make_unique<WwiseReader>(Ar, source, entry.Size);
                        bank->Path = entry.AudioPath;
                        LoadedSize += bank->LoadedSize;

                        for (const auto& embeddedWem : bank->WwiseEncodedMedias)
                            WwiseEncodedMedias[embeddedWem.first] = embeddedWem.second;

                        AKPKBankEntries.push_back(std::move(bank));
                    }

                    for (AkEntry& entry : AKPKWemEntries)
                    {
                        entry.ReadAudioPath(folders);
                        LoadedSize += entry.ReadData(Ar, source);
                        WwiseEncodedMedias[entry.Name()] = entry.Data;
                    }

                    // return cause we got everything else from entries
                    return;
                }
                case EChunkID::BankHeader:
                {
                    LoadedSize += sectionLength;
                    Header = AkBankHeader(Ar, sectionLength);

                    Ar.Version = Header.Version;
                    Ar.HasFeedback = Header.FeedbackInBank;
                    // C# warns when the version is unsupported; the port has no logging layer.
                    break;
                }
                case EChunkID::BankInit:
                    LoadedSize += sectionLength;
                    AKPluginList = Ar.ReadMap<uint32_t, std::string>(
                        [&Ar] { return Ar.Read<uint32_t>(); },
                        [&Ar] { return Ar.Version <= 136 ? Ar.ReadFString() : Ar.ReadStzString(); });
                    break;
                case EChunkID::BankDataIndex:
                    LoadedSize += sectionLength;
                    WemIndexes = Ar.ReadArray<MediaHeader>(sectionLength / 12);
                    break;
                case EChunkID::BankData:
                {
                    if (WemIndexes.empty()) break;

                    for (const MediaHeader& wemData : WemIndexes)
                    {
                        if (wemData.Id == 0) continue;

                        auto temp = ReadDeferredByteData(Ar, source, position + wemData.Offset, wemData.Size);
                        LoadedSize += temp->LoadedSize();
                        WwiseEncodedMedias[std::to_string(wemData.Id)] = std::move(temp);
                    }
                    break;
                }
                case EChunkID::BankHierarchy:
                    LoadedSize += sectionLength;
                    Hierarchies = Ar.ReadArrayWith([&Ar] { return Hierarchy(Ar); });
                    break;
                case EChunkID::RIFF:
                    if (Ar.Position + sectionLength > Ar.Length)
                        throw RIFFSectionSizeException();
                    Ar.Position -= 8;
                    WemFile = ReadDeferredByteData(Ar, source, Ar.Position, 8 + sectionLength);
                    LoadedSize += WemFile->LoadedSize();
                    break;
                case EChunkID::BankStrMap:
                    LoadedSize += sectionLength;
                    Ar.Position += 4; // var type = Ar.Read<AKBKStringType>;
                    BankIDToFileName = Ar.ReadMap<uint32_t, std::string>(
                        [&Ar] { return Ar.Read<uint32_t>(); },
                        [&Ar] { return Ar.ReadString(); });
                    break;
                case EChunkID::BankStateMg:
                    if (Ar.IsSupported()) // Let's guard this just in case
                    {
                        LoadedSize += sectionLength;
                        GlobalSettings_.emplace(Ar);
                    }
                    break;
                case EChunkID::BankEnvSetting:
                    if (Ar.IsSupported()) // Let's guard this just in case
                    {
                        LoadedSize += sectionLength;
                        EnvSettings.emplace(Ar);
                    }
                    break;
                case EChunkID::FXPR:
                    break;
                case EChunkID::BankCustomPlatformName:
                {
                    LoadedSize += sectionLength;
                    if (Ar.Version <= 136)
                    {
                        // A length-prefixed byte array read as ASCII, with the trailing NULs trimmed.
                        const std::vector<uint8_t> bytes = Ar.ReadArrayCounted<uint8_t>();
                        Platform.assign(bytes.begin(), bytes.end());
                        while (!Platform.empty() && Platform.back() == '\0') Platform.pop_back();
                    }
                    else Platform = Ar.ReadStzString();
                    break;
                }
                case EChunkID::PLUGIN:
                    // Plugin container holds audio data encoded specifically for a given Wwise plugin
                    // For example: ADM3 codec (Crankcase Audio), AK Convolution Reverb impulse response
                    Ar.Position -= 8;
                    WemFile = ReadDeferredByteData(Ar, source, Ar.Position, 8 + sectionLength);
                    LoadedSize += WemFile->LoadedSize();
                    IsPlugin = true;
                    break;
                case EChunkID::MIDI:
                    Ar.Position -= 8;
                    MidiData = ReadDeferredByteData(Ar, source, Ar.Position, 8 + sectionLength);
                    LoadedSize += MidiData->LoadedSize();
                    break;
                default:
                    // C# warns about the unknown tag here; the port has no logging layer.
                    break;
            }

            // Whatever the arm did, the next section starts at the declared end of this one.
            if (Ar.Position != position + sectionLength)
            {
                Ar.Position = position + sectionLength;
            }
        }
    }

    std::shared_ptr<FDeferredByteData> WwiseReader::ReadDeferredByteData(
        Readers::FArchive& Ar, const WwiseDataSource& source, int64_t offset, int32_t size)
    {
        if (source.Kind == WwiseDataSource::EKind::BulkData && Ar.SupportPartialReads() &&
            source.AssetAr != nullptr && source.BulkData != nullptr)
        {
            auto newBulkData = std::make_shared<FBulkDataDeferredByteData>(*source.AssetAr, *source.BulkData, offset, size);
            Ar.Position = offset + size;
            return newBulkData;
        }

        if (source.Kind == WwiseDataSource::EKind::GameFile && Ar.SupportPartialReads() && source.File != nullptr)
        {
            // The GameFile is borrowed from the source, so the shared_ptr must not own it.
            std::shared_ptr<FileProvider::Objects::GameFile> borrowed(source.File, [](FileProvider::Objects::GameFile*) {});
            auto gameFileData = std::make_shared<FGameFileDeferredByteData>(std::move(borrowed), offset, size);
            Ar.Position = offset + size;
            return gameFileData;
        }

        Ar.Position = offset;
        return std::make_shared<FArrayDeferredByteData>(Ar.ReadBytes(size));
    }

    std::optional<uint32_t> WwiseReader::TryReadSoundBankId(Readers::FArchive& Ar)
    {
        while (Ar.Position < Ar.Length)
        {
            const auto sectionIdentifier = Ar.Read<EChunkID>();
            const int32_t sectionLength = Ar.Read<int32_t>();
            const int64_t sectionStart = Ar.Position;

            if (sectionIdentifier == EChunkID::BankHeader)
            {
                Ar.Read<uint32_t>();          // Version
                return Ar.Read<uint32_t>();   // SoundBankId
            }

            Ar.Position = sectionStart + sectionLength;
        }

        return std::nullopt;
    }
}
