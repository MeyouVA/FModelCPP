// Ported from CUE4Parse/UE4/FMod/FModProvider.cs — see FModProvider.h for the deliberate differences.
#include "FModProvider.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <map>

#include "Utils/EventNodesResolver.h"
#include "../Readers/FByteArchive.h"
#include "../Assets/Exports/PropertyUtil.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../../FileProvider/Objects/GameFile.h"
#include "../../UE4Config/Parsing/IniToken.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::FMod
{
    namespace PropertyUtil = CUE4Parse::UE4::Assets::Exports::PropertyUtil;
    using CUE4Parse::FileProvider::Objects::GameFile;
    using CUE4Parse::UE4::FMod::Utils::EventNodesResolver;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using UE4Config::Parsing::InstructionToken;

    std::optional<std::vector<uint8_t>> FModProvider::_encryptionKey;

    namespace
    {
        bool EqualsIgnoreCase(const std::string& a, const std::string& b)
        {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++)
            {
                const char ca = (a[i] >= 'A' && a[i] <= 'Z') ? static_cast<char>(a[i] + 32) : a[i];
                const char cb = (b[i] >= 'A' && b[i] <= 'Z') ? static_cast<char>(b[i] + 32) : b[i];
                if (ca != cb) return false;
            }
            return true;
        }

        bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
        {
            if (needle.size() > haystack.size()) return false;
            for (size_t i = 0; i + needle.size() <= haystack.size(); i++)
            {
                if (EqualsIgnoreCase(haystack.substr(i, needle.size()), needle)) return true;
            }
            return false;
        }

        // C#'s Regex.Unescape over the ini value: the StudioBankKey is written with backslash escapes so a
        // key with non-printable bytes survives the ini round trip. Only the escapes .NET actually emits
        // there are handled; anything else keeps the character after the backslash, as Regex.Unescape does.
        std::string RegexUnescape(const std::string& value)
        {
            std::string result;
            result.reserve(value.size());
            for (size_t i = 0; i < value.size(); i++)
            {
                if (value[i] != '\\' || i + 1 >= value.size()) { result.push_back(value[i]); continue; }

                const char escape = value[++i];
                switch (escape)
                {
                    case 'a': result.push_back('\a'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'v': result.push_back('\v'); break;
                    case '0': result.push_back('\0'); break;
                    case 'x':
                        if (i + 2 < value.size())
                        {
                            result.push_back(static_cast<char>(std::stoi(value.substr(i + 1, 2), nullptr, 16)));
                            i += 2;
                        }
                        break;
                    default: result.push_back(escape); break;
                }
            }
            return result;
        }

        std::string Trim(const std::string& s, char c)
        {
            size_t start = 0;
            size_t end = s.size();
            while (start < end && s[start] == c) ++start;
            while (end > start && s[end - 1] == c) --end;
            return s.substr(start, end - start);
        }
    }

    FModProvider::FModProvider(AbstractFileProvider& provider, const std::string& gameDirectory)
    {
        LoadFModSettings(provider);
        LoadPakBanks(provider);
        LoadFileBanks(gameDirectory);
        UpdateEventCache();
    }

    void FModProvider::MergeIntoCache(std::unique_ptr<FModReader> mergedBank)
    {
        if (mergedBank == nullptr) return;

        const FModGuid guid = mergedBank->GetBankGuid();
        const auto existing = _mergedReaders.find(guid);
        if (existing != _mergedReaders.end())
        {
            existing->second->Merge(*mergedBank);
        }
        else
        {
            _mergedReaders[guid] = std::move(mergedBank);
        }
    }

    void FModProvider::LoadPakBanks(AbstractFileProvider& provider)
    {
        // C#'s GroupBy(x => x.Name.SubstringBefore('.')): "Master.bank" and "Master.assets.bank" are one
        // logical bank. Ordered so the merge order is reproducible, where C#'s grouping order is incidental.
        std::map<std::string, std::vector<std::shared_ptr<GameFile>>> banks;
        provider.Files.ForEach([&](const std::string&, const std::shared_ptr<GameFile>& file)
        {
            if (file->Extension() == "bank" && ContainsIgnoreCase(file->Path(), "FMOD"))
                banks[CUE4Parse::Utils::SubstringBefore(file->Name(), '.')].push_back(file);
        });

        for (const auto& [groupKey, group] : banks)
        {
            std::unique_ptr<FModReader> mergedBank;
            for (const auto& file : group)
            {
                const std::optional<std::vector<uint8_t>> data = provider.TrySaveAsset(file->Path());
                if (!data.has_value()) continue;

                Readers::FByteArchive Ar(file->Name(), *data, provider.Versions);
                std::unique_ptr<FModReader> fmodBank = TryLoadBank(Ar, file->Name());
                if (fmodBank == nullptr)
                {
                    // C# logs "Failed to serialize FMOD Bank file {bank}".
                    continue;
                }

                if (mergedBank == nullptr)
                {
                    mergedBank = std::move(fmodBank);
                }
                else
                {
                    mergedBank->Merge(*fmodBank);
                }
            }

            MergeIntoCache(std::move(mergedBank));
        }
    }

    void FModProvider::LoadFileBanks(std::string gameDirectory)
    {
        namespace fs = std::filesystem;
        std::error_code ec;

        fs::path dir(gameDirectory);
        if (!EqualsIgnoreCase(dir.filename().string(), "Paks"))
            return;

        if (dir.has_parent_path())
            dir = dir.parent_path();

        fs::path fmodDir;
        if (!_bankOutputDirectory.empty())
        {
            const fs::path potentialPath = dir / _bankOutputDirectory;
            if (fs::exists(potentialPath, ec) && fs::is_directory(potentialPath, ec))
                fmodDir = potentialPath;
        }

        if (fmodDir.empty())
        {
            // C#'s EnumerateDirectories(gameDirectory, "FMOD", AllDirectories).SelectMany(GetDirectories
            // "Desktop").FirstOrDefault: the first "Desktop" under any "FMOD" folder.
            for (fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end;
                 it != end && !ec; it.increment(ec))
            {
                if (!it->is_directory(ec) || it->path().filename() != "FMOD") continue;
                std::error_code inner;
                for (fs::recursive_directory_iterator jt(it->path(),
                         fs::directory_options::skip_permission_denied, inner), jend;
                     jt != jend && !inner; jt.increment(inner))
                {
                    if (jt->is_directory(inner) && jt->path().filename() == "Desktop") { fmodDir = jt->path(); break; }
                }
                if (!fmodDir.empty()) break;
            }
        }

        if (fmodDir.empty())
        {
            // C# warns "FMOD Desktop directory not found under {0}".
            return;
        }

        std::map<std::string, std::vector<fs::path>> banks;
        for (fs::recursive_directory_iterator it(fmodDir, fs::directory_options::skip_permission_denied, ec), end;
             it != end && !ec; it.increment(ec))
        {
            if (!it->is_regular_file(ec)) continue;
            const std::string path = it->path().string();
            if (path.size() < 5 || path.compare(path.size() - 5, 5, ".bank") != 0) continue;
            banks[CUE4Parse::Utils::SubstringBefore(it->path().filename().string(), '.')].push_back(it->path());
        }

        for (const auto& [groupKey, group] : banks)
        {
            std::unique_ptr<FModReader> mergedBank;
            for (const fs::path& file : group)
            {
                // C# hands File.OpenRead(file) to a BinaryReader; the port has no stream archive, so the
                // file is read whole into an FByteArchive. Banks are small enough that this is a wash.
                std::vector<uint8_t> data;
                {
                    std::error_code sizeEc;
                    const auto size = fs::file_size(file, sizeEc);
                    if (sizeEc) continue;
                    data.resize(static_cast<size_t>(size));
                    FILE* handle = nullptr;
                    if (fopen_s(&handle, file.string().c_str(), "rb") != 0 || handle == nullptr) continue;
                    const size_t read = std::fread(data.data(), 1, data.size(), handle);
                    std::fclose(handle);
                    data.resize(read);
                }

                const std::string bankName = file.stem().string();
                Readers::FByteArchive Ar(bankName, std::move(data));
                std::unique_ptr<FModReader> fmodBank = TryLoadBank(Ar, bankName);
                if (fmodBank == nullptr)
                {
                    // C# logs "Failed to serialize FMOD Bank file {bank}".
                    continue;
                }

                if (mergedBank == nullptr)
                {
                    mergedBank = std::move(fmodBank);
                }
                else
                {
                    mergedBank->Merge(*fmodBank);
                }
            }

            MergeIntoCache(std::move(mergedBank));
        }
    }

    void FModProvider::LoadFModSettings(AbstractFileProvider& provider)
    {
        const auto& engineConfig = provider.DefaultEngine;

        std::vector<std::string> values;
        engineConfig.EvaluatePropertyValues("/Script/FMODStudio.FMODSettings", "BankOutputDirectory", values);
        const std::string path =
            values.empty() ? std::string()
                           : CUE4Parse::Utils::SubstringBefore(CUE4Parse::Utils::SubstringAfter(values.front(), "Path=\""), "\")");
        if (!path.empty())
        {
            // C# maps both separators onto Path.DirectorySeparatorChar; std::filesystem accepts '/' on every
            // platform, so the path is normalised onto that instead.
            _bankOutputDirectory = path;
            std::replace(_bankOutputDirectory.begin(), _bankOutputDirectory.end(), '\\', '/');
        }

        const auto* fmodSection = engineConfig.FindSection("/Script/FMODStudio.FMODSettings");

        const InstructionToken* token = nullptr;
        if (fmodSection != nullptr)
        {
            for (const auto& t : fmodSection->Tokens)
            {
                const auto* instruction = dynamic_cast<const InstructionToken*>(t.get());
                if (instruction != nullptr && instruction->Key == "StudioBankKey") { token = instruction; break; }
            }
        }

        if (token != nullptr && !token->Value.empty())
        {
            const std::string key = RegexUnescape(Trim(token->Value, '"'));
            _encryptionKey = std::vector<uint8_t>(key.begin(), key.end());
            // C# logs the key it found at Information level. Not reproduced -- it is a decryption key.
        }
        // C#'s #if DEBUG "encryption key not found" note is omitted with the logging layer.
    }

    std::unique_ptr<FModReader> FModProvider::TryLoadBank(Readers::FArchive& Ar, const std::string& bankName) const
    {
        try
        {
            return std::make_unique<FModReader>(Ar, bankName, _encryptionKey);
        }
        catch (const std::exception&)
        {
            // C# logs "Can't load FMOD bank".
            return nullptr;
        }
    }

    void FModProvider::UpdateEventCache()
    {
        std::unordered_map<FModGuid, std::vector<FWaveformRef>> eventSamples;

        for (const auto& [bankGuid, fmodReader] : _mergedReaders)
        {
            bool isFullyResolved = false;
            const auto resolvedEvents = EventNodesResolver::TryResolveAudioEvents(*fmodReader, isFullyResolved);

            // C#'s #if DEBUG EventNodesResolver.LogMissingSamples call is omitted with the logging layer.

            for (const auto& kvp : resolvedEvents)
            {
                _eventResolutionStatus.emplace(kvp.first, isFullyResolved); // C#'s TryAdd: first write wins
                _eventToReaderMap[kvp.first] = fmodReader->GetBankGuid();

                if (kvp.second.empty())
                    continue;

                auto& sampleList = eventSamples[kvp.first];
                sampleList.insert(sampleList.end(), kvp.second.begin(), kvp.second.end());
            }
        }

        _resolvedEventsCache = std::move(eventSamples);
    }

    std::vector<FModExtractedSound> FModProvider::ExtractEventSounds(UFMODEvent& audioEvent)
    {
        FGuid fguid;
        if (!PropertyUtil::TryGet<FGuid>(audioEvent, "AssetGuid", fguid)) return {};
        const FModGuid eventGuid(fguid);

        const auto cached = _resolvedEventsCache.find(eventGuid);
        if (cached == _resolvedEventsCache.end())
        {
            // There's no way of associating events with samples from sound table, so we just provide all sounds
            // from sound table only if all samples were resolved because if they weren't it might be an issue
            // on our side
            const auto status = _eventResolutionStatus.find(eventGuid);
            if (status != _eventResolutionStatus.end() && status->second)
            {
                const auto readerGuid = _eventToReaderMap.find(eventGuid);
                if (readerGuid != _eventToReaderMap.end())
                {
                    const auto reader = _mergedReaders.find(readerGuid->second);
                    if (reader != _mergedReaders.end())
                        return ExtractBankSoundTable(*reader->second);
                }
            }

            // C# warns "Can't find FMODEvent with the guid {0}".
            return {};
        }

        const auto readerGuid = _eventToReaderMap.find(eventGuid);
        const FModReader* owner = nullptr;
        if (readerGuid != _eventToReaderMap.end())
        {
            const auto reader = _mergedReaders.find(readerGuid->second);
            if (reader != _mergedReaders.end()) owner = reader->second.get();
        }
        if (owner == nullptr) return {};

        return ExtractAudioSamples(cached->second, audioEvent.Name, *owner);
    }

    std::vector<FModExtractedSound> FModProvider::ExtractBankSounds(UFMODBank& audioBank)
    {
        const FGuid assetGuid = PropertyUtil::GetOrDefault<FGuid>(audioBank, "AssetGuid");
        const FModGuid bankGuid(assetGuid);

        const auto bank = _mergedReaders.find(bankGuid);
        if (bank == _mergedReaders.end())
        {
            // C# warns "Can't find FMODBank with the guid {0}".
            return {};
        }

        const std::vector<FWaveformRef> samples = bank->second->ExtractTracks();

        return ExtractAudioSamples(samples, audioBank.Name, *bank->second);
    }

    std::vector<FModExtractedSound> FModProvider::ExtractBankSoundTable(const FModReader& fmodReader)
    {
        return ExtractAudioSamples(fmodReader.ExtractSoundTableTracks(), fmodReader.BankName, fmodReader);
    }

    std::vector<FModExtractedSound> FModProvider::ExtractBankSounds(const FModReader& fmodReader)
    {
        return ExtractAudioSamples(fmodReader.ExtractTracks(), fmodReader.BankName, fmodReader);
    }

    std::vector<FModExtractedSound> FModProvider::ExtractAudioSamples(const std::vector<FWaveformRef>& samples,
                                                                     const std::string& fallbackSampleName,
                                                                     const FModReader& owner) const
    {
        std::vector<FModExtractedSound> extracted;
        extracted.reserve(samples.size());
        for (size_t i = 0; i < samples.size(); i++)
        {
            const FWaveformRef& sample = samples[i];
            // C# calls sample.RebuildAsStandardFileFormat here and skips the sample when it fails, which is
            // where the bytes and the file extension come from. Without that decoder there is nothing to
            // rebuild, so what is checked instead is that the reference actually names a subsound that
            // exists -- the same "this one is unusable, skip it" filter, applied one step earlier.
            if (sample.SoundBankIndex < 0 ||
                sample.SoundBankIndex >= static_cast<int32_t>(owner.SoundBankData.size()))
                continue;
            const FModSoundBank& soundBank = owner.SoundBankData[static_cast<size_t>(sample.SoundBankIndex)];
            if (sample.SubsoundIndex < 0 || sample.SubsoundIndex >= soundBank.SampleCount)
                continue;

            // C#'s `sample.Name ?? $"{fallbackSampleName}_{i}"`. Sample names live in the FSB5 name table,
            // which only the decoder reads, so the fallback is always what is used here.
            extracted.push_back(FModExtractedSound{
                fallbackSampleName + "_" + std::to_string(i), sample, &soundBank});
        }

        return extracted;
    }
}
