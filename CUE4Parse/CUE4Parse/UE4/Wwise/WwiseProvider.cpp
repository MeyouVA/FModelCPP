// Ported from CUE4Parse/UE4/Wwise/WwiseProvider.cs — see WwiseProvider.h for the deliberate differences.
#include "WwiseProvider.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>

#include "WwiseFnv.h"
#include "Enums/EAKBKHircType.h"
#include "Enums/EAkActionType.h"
#include "Enums/EAkPluginType.h"
#include "Objects/HIRC/Containers/HierarchyEvent.h"
#include "Objects/HIRC/Containers/HierarchyEventAction.h"
#include "Objects/HIRC/Containers/HierarchyFxVariants.h"
#include "Objects/HIRC/Containers/HierarchyLayerContainer.h"
#include "Objects/HIRC/Containers/HierarchyMusicRandomSequenceContainer.h"
#include "Objects/HIRC/Containers/HierarchyMusicSegment.h"
#include "Objects/HIRC/Containers/HierarchyMusicSwitchContainer.h"
#include "Objects/HIRC/Containers/HierarchyMusicTrack.h"
#include "Objects/HIRC/Containers/HierarchyRandomSequenceContainer.h"
#include "Objects/HIRC/Containers/HierarchySoundSfxVoice.h"
#include "../Assets/IPackage.h"
#include "../Assets/ResolvedObject.h"
#include "../Assets/Exports/PropertyUtil.h"
#include "../Assets/Exports/Wwise/UWwiseAssetLibrary.h"
#include "../Assets/Objects/Properties/ObjectProperty.h"
#include "../Objects/UObject/FPackageFileSummary.h"
#include "../Objects/UObject/ObjectResource.h"
#include "../../FileProvider/Objects/OsGameFile.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::Wwise
{
    namespace PropertyUtil = CUE4Parse::UE4::Assets::Exports::PropertyUtil;
    using CUE4Parse::UE4::Assets::Exports::Wwise::UWwiseAssetLibrary;
    using CUE4Parse::UE4::Wwise::Enums::EAKBKHircType;
    using CUE4Parse::UE4::Wwise::Enums::EAkActionType;
    using CUE4Parse::UE4::Wwise::Enums::EAkPluginType;
    namespace C = CUE4Parse::UE4::Wwise::Objects::HIRC::Containers;

    namespace
    {
        std::string ToLowerInvariant(std::string s)
        {
            for (char& c : s)
                if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            return s;
        }

        // Path.Combine, with '/' as the inserted separator (the convention set in AkEntry.h): an empty first
        // part yields just the second, and no separator is added when one is already there.
        std::string PathCombine(const std::string& first, const std::string& second)
        {
            if (first.empty()) return second;
            if (second.empty()) return first;
            const char last = first.back();
            return first + ((last == '/' || last == '\\') ? "" : "/") + second;
        }

        // Path.GetDirectoryName: everything before the last separator, either flavour.
        std::string GetDirectoryName(const std::string& path)
        {
            const size_t sep = path.find_last_of("/\\");
            return sep == std::string::npos ? std::string() : path.substr(0, sep);
        }

        // Path.GetFileNameWithoutExtension.
        std::string GetFileNameWithoutExtension(const std::string& path)
        {
            const size_t sep = path.find_last_of("/\\");
            const std::string name = sep == std::string::npos ? path : path.substr(sep + 1);
            return Utils::SubstringBeforeLast(name, '.');
        }

        std::string Replace(std::string s, char from, char to)
        {
            std::replace(s.begin(), s.end(), from, to);
            return s;
        }

        // C#'s `uint.TryParse`: digits only, and it must fit. Wem file names are bare ids.
        bool TryParseUInt(const std::string& text, uint32_t& value)
        {
            if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) return false;
            try
            {
                const unsigned long long parsed = std::stoull(text);
                if (parsed > 0xFFFFFFFFull) return false;
                value = static_cast<uint32_t>(parsed);
                return true;
            }
            catch (const std::exception&) { return false; }
        }

        bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
        {
            if (needle.empty()) return true;
            const std::string h = ToLowerInvariant(haystack);
            const std::string n = ToLowerInvariant(needle);
            return h.find(n) != std::string::npos;
        }

        bool EqualsIgnoreCase(const std::string& a, const std::string& b)
        {
            return a.size() == b.size() && ToLowerInvariant(a) == ToLowerInvariant(b);
        }
    }

    std::string WwiseExtractedSound::ToString() const
    {
        return OutputPath + "." + ToLowerInvariant(Extension);
    }

    const std::unordered_set<std::string>& WwiseProvider::ValidWwiseExtensions()
    {
        static const std::unordered_set<std::string> set{"bnk", "pck", "wem"};
        return set;
    }

    WwiseProvider::WwiseProvider(AbstractVfsFileProvider& provider, std::string gameDirectory)
        : _provider(provider), _gameDirectory(std::move(gameDirectory))
    {
        _baseWwiseAudioPath = PathCombine(PathCombine(_provider.ProjectName(), "Content"), "WwiseAudio");

        LoadMultiReferenceLibrary();

        BulkInitializeWwise();
        if (!_completedWwiseFullBnkInit)
            throw std::runtime_error(
                "Failed to initialize Wwise soundbanks. Ensure that the provider has files to work with.");
    }

    std::vector<WwiseExtractedSound> WwiseProvider::ExtractBankSounds(WwiseReader& wwiseReader)
    {
        CacheWwiseFile(wwiseReader);
        const std::string ownerDirectory = Utils::SubstringBeforeLast(wwiseReader.Path, '.');

        // C# returns early when WwiseEncodedMedias is null; the port's map is never null, and an empty map
        // produces the same empty result.
        std::vector<WwiseExtractedSound> results;
        results.reserve(wwiseReader.WwiseEncodedMedias.size());
        for (const auto& media : wwiseReader.WwiseEncodedMedias)
        {
            std::shared_ptr<FDeferredByteData> data = media.second;
            uint32_t id = 0;
            if (TryParseUInt(media.first, id))
            {
                const auto it = _looseWemFilesLookup.find(id);
                if (it != _looseWemFilesLookup.end() && it->second != nullptr && it->second->IsValid())
                    data = it->second;
            }

            results.push_back(WwiseExtractedSound{PathCombine(ownerDirectory, media.first), "wem", data});
        }

        return results;
    }

    std::vector<WwiseExtractedSound> WwiseProvider::ExtractBankSounds(UAkAudioBank& audioBank)
    {
        if (!audioBank.SoundBankCookedData.has_value())
        {
            const uint32_t soundBankId = PropertyUtil::GetOrDefault<uint32_t>(audioBank, "ShortID");

            if (soundBankId == 0)
                return {};

            WwiseReader* soundBank = LoadSoundBankById(soundBankId, /*returnBank*/ true);

            if (soundBank == nullptr)
                return {};

            return ExtractBankSounds(*soundBank);
        }

        const std::string ownerDirectory = GetOwnerDirectory(audioBank);
        std::vector<WwiseExtractedSound> results;
        for (const auto& eventName : audioBank.SoundBankCookedData->IncludedEventNames)
        {
            if (eventName.IsNone())
                continue;

            const uint32_t audioEventId = WwiseFnv::GetHash(eventName.Text());
            LoopThroughEvent(audioEventId, results, ownerDirectory, eventName.Text());
        }

        return results;
    }

    std::vector<WwiseExtractedSound> WwiseProvider::ExtractAudioEventSounds(UAkAudioEvent& audioEvent)
    {
        std::vector<WwiseExtractedSound> results;

        const std::string ownerDirectory = GetOwnerDirectory(audioEvent);
        if (!audioEvent.EventCookedData.has_value())
        {
            const auto* requiredBankProp = PropertyUtil::FindTag(audioEvent.Properties, "RequiredBank");
            const auto* shortIdProp = PropertyUtil::FindTag(audioEvent.Properties, "ShortID");

            uint32_t audioEventId = 0;
            const bool haveShortId = shortIdProp != nullptr && shortIdProp->Tag != nullptr &&
                                     PropertyUtil::PropertyValue(*shortIdProp->Tag, audioEventId);
            if (!haveShortId || requiredBankProp == nullptr || requiredBankProp->Tag == nullptr)
            {
                audioEventId = WwiseFnv::GetHash(audioEvent.Name);
            }

            std::string soundBankId;
            if (requiredBankProp != nullptr && requiredBankProp->Tag != nullptr)
            {
                const auto* objProp = dynamic_cast<const Assets::Objects::Properties::ObjectProperty*>(
                    requiredBankProp->Tag.get());
                if (objProp != nullptr && !objProp->Value.IsNull())
                {
                    // C#'s objProp.Value.TryLoad(out var audioBank). RequiredBank is normally an *import*
                    // (the bank lives in another package), and this port's ResolvedImportObject::Object()
                    // still returns null — see ResolvedObject.h — so this resolves only for the rarer
                    // same-package case. Until import loading lands, the miss falls through to the FNV hash
                    // above, exactly as a failed TryLoad does in C#.
                    Assets::ResolvedObject* resolved =
                        objProp->Value.Owner != nullptr
                            ? objProp->Value.Owner->ResolvePackageIndex(&objProp->Value)
                            : nullptr;
                    Assets::Exports::UObject* audioBank = resolved != nullptr ? resolved->Load() : nullptr;
                    if (audioBank != nullptr)
                    {
                        const auto* shortId = PropertyUtil::FindTag(audioBank->Properties, "ShortID");
                        uint32_t value = 0;
                        if (shortId != nullptr && shortId->Tag != nullptr &&
                            PropertyUtil::PropertyValue(*shortId->Tag, value))
                            soundBankId = std::to_string(value);
                    }
                }
            }

            if (!soundBankId.empty())
                LoadSoundBankById(static_cast<uint32_t>(std::stoul(soundBankId)));

            LoopThroughEvent(audioEventId, results, ownerDirectory, audioEvent.Name);

            return results;
        }

        const auto& wwiseData = *audioEvent.EventCookedData;

        // cache all banks first
        for (const auto& [languageData, eventData] : wwiseData.EventLanguageMap)
        {
            if (!eventData.has_value())
                continue;

            for (const auto& media : eventData->Media)
            {
                CacheMediaCookedData(media);
            }

            for (const auto& soundBank : eventData->SoundBanks)
            {
                CacheSoundBankCookedData(soundBank);
            }

            for (const auto& leaf : eventData->SwitchContainerLeaves)
            {
                for (const auto& soundBank : leaf.SoundBanks)
                {
                    CacheSoundBankCookedData(soundBank);
                }
            }
        }

        // Track what's in media first so we don't resolve the same audio twice via event resolution
        std::unordered_set<uint32_t> visitedMedia;
        for (const auto& [languageData, eventData] : wwiseData.EventLanguageMap)
        {
            if (!eventData.has_value())
                continue;

            for (const auto& media : eventData->Media)
            {
                if (!visitedMedia.insert(media.MediaId).second)
                    continue;
                ProcessMediaCookedData(ownerDirectory, media, languageData, results);
            }

            for (const auto& leaf : eventData->SwitchContainerLeaves)
            {
                for (const auto& media : leaf.Media)
                {
                    if (!visitedMedia.insert(media.MediaId).second)
                        continue;
                    ProcessMediaCookedData(ownerDirectory, media, languageData, results);
                }
            }
        }

        for (const auto& [languageData, eventData] : wwiseData.EventLanguageMap)
        {
            if (!eventData.has_value())
                continue;

            // Faithful quirk: both loops ignore their loop variable and re-walk the *event*, so an event
            // with N sound banks resolves the same event N times. Kept as-is.
            for (const auto& soundBank : eventData->SoundBanks)
            {
                (void) soundBank;
                ProcessSoundBankCookedData(ownerDirectory, *eventData, results, visitedMedia);
            }

            for (const auto& leaf : eventData->SwitchContainerLeaves)
            {
                for (const auto& soundBank : leaf.SoundBanks)
                {
                    (void) soundBank;
                    ProcessSoundBankCookedData(ownerDirectory, *eventData, results, visitedMedia);
                }
            }
        }

        return results;
    }

    void WwiseProvider::ProcessMediaCookedData(const std::string& ownerDirectory, const FWwiseMediaCookedData& media,
                                               const FWwiseLanguageCookedData& languageData,
                                               std::vector<WwiseExtractedSound>& results)
    {
        const std::string mediaPath = media.MediaPathName.IsNone()
            ? (media.PackagedFile != nullptr ? media.PackagedFile->PathName.ToString() : std::string())
            : media.MediaPathName.Text();
        const std::string wemFileName = GetFileNameWithoutExtension(mediaPath);

        // C#'s switch expression over the four sources, in the same order of preference.
        std::shared_ptr<FDeferredByteData> data;
        if (media.PackagedFile != nullptr && media.PackagedFile->BulkData != nullptr &&
            media.PackagedFile->BulkData->WemFile != nullptr && media.PackagedFile->BulkData->WemFile->IsValid())
        {
            data = media.PackagedFile->BulkData->WemFile;
        }
        else if (const auto wem = _looseWemFilesLookup.find(media.MediaId); wem != _looseWemFilesLookup.end())
        {
            data = wem->second;
        }
        else if (const auto encoded = _wwiseEncodedMedia.find(wemFileName); encoded != _wwiseEncodedMedia.end())
        {
            data = encoded->second;
        }
        else if (const auto multiRef =
                     _multiReferenceLibraryCache.find(media.PackagedFile != nullptr ? media.PackagedFile->Hash : 0u);
                 multiRef != _multiReferenceLibraryCache.end())
        {
            data = multiRef->second != nullptr ? multiRef->second->WemFile : nullptr;
        }

        // C# logs "Failed to load data for '{WemFileName}' wem loose file" when data is null and still adds
        // the entry; the entry is kept here too.

        const std::string mediaDebugName = !media.DebugName.Text().empty() && !media.DebugName.IsNone()
            ? Utils::SubstringBeforeLast(media.DebugName.Text(), '.')
            : wemFileName;

        const std::string namedPath =
            PathCombine(ownerDirectory, mediaDebugName + " (" + languageData.LanguageName.Text() + ")");

        results.push_back(WwiseExtractedSound{Replace(namedPath, '\\', '/'), "wem", data});
    }

    void WwiseProvider::CacheSoundBankCookedData(const FWwiseSoundBankCookedData& soundBank)
    {
        WwiseReader* bulkPackagedSoundBank =
            soundBank.PackagedFile != nullptr ? soundBank.PackagedFile->BulkData.get() : nullptr;
        if (bulkPackagedSoundBank != nullptr &&
            _wwiseLoadedSoundBanks.find(bulkPackagedSoundBank->Header.SoundBankId) == _wwiseLoadedSoundBanks.end())
        {
            CacheWwiseFile(*bulkPackagedSoundBank);
            _wwiseLoadedSoundBanks.insert(bulkPackagedSoundBank->Header.SoundBankId);
        }
    }

    void WwiseProvider::CacheMediaCookedData(const FWwiseMediaCookedData& media)
    {
        WwiseReader* bulkPackagedMedia =
            media.PackagedFile != nullptr ? media.PackagedFile->BulkData.get() : nullptr;
        if (bulkPackagedMedia != nullptr && bulkPackagedMedia->WemFile != nullptr &&
            bulkPackagedMedia->WemFile->IsValid())
        {
            _wwiseEncodedMedia[std::to_string(media.MediaId)] = bulkPackagedMedia->WemFile;
        }
    }

    void WwiseProvider::ProcessSoundBankCookedData(const std::string& ownerDirectory,
                                                   const FWwiseEventCookedData& eventData,
                                                   std::vector<WwiseExtractedSound>& results,
                                                   const std::unordered_set<uint32_t>& visitedMedia)
    {
        LoopThroughEvent(eventData.EventId, results, ownerDirectory, visitedMedia, eventData.DebugName.Text());
    }

    WwiseReader* WwiseProvider::LoadSoundBankById(uint32_t soundBankId, bool returnBank)
    {
        if (!returnBank && _wwiseLoadedSoundBanks.find(soundBankId) != _wwiseLoadedSoundBanks.end())
            return nullptr;

        // C#'s `_validWwiseExtensions.Except(["wem"])`: bank containers only.
        const std::string basePath = Replace(_baseWwiseAudioPath, '\\', '/');

        WwiseReader* found = nullptr;
        _provider.Files.ForEach([&](const std::string& key, const std::shared_ptr<GameFile>& file)
        {
            if (found != nullptr) return;
            if (!ContainsIgnoreCase(key, basePath))
                return;
            const std::string extension = ToLowerInvariant(Utils::SubstringAfterLast(key, '.'));
            if (extension != "bnk" && extension != "pck")
                return;

            try
            {
                auto reader = file->CreateReader();
                if (reader == nullptr) return;
                const std::optional<uint32_t> id = WwiseReader::TryReadSoundBankId(*reader);
                if (!id.has_value() || *id != soundBankId)
                    return;

                reader->Position = 0;
                FWwiseArchive wwiseAr(*reader);
                auto soundBank = std::make_unique<WwiseReader>(wwiseAr, WwiseDataSource::FromGameFile(*file));
                WwiseReader* raw = soundBank.get();
                _ownedReaders.push_back(std::move(soundBank));
                CacheWwiseFile(*raw);
                _wwiseLoadedSoundBanks.insert(soundBankId);
                found = raw;
            }
            catch (const std::exception&)
            {
                // C# warns "Failed to read soundbank file '{key}'".
            }
        });

        // C# warns "Soundbank with ID {ID} wasn't found" on a miss.
        return found;
    }

    void WwiseProvider::LoopThroughEvent(uint32_t eventId, std::vector<WwiseExtractedSound>& results,
                                         const std::string& ownerDirectory, const std::string& debugName)
    {
        LoopThroughEvent(eventId, results, ownerDirectory, std::unordered_set<uint32_t>(), debugName);
    }

    void WwiseProvider::LoopThroughEvent(uint32_t eventId, std::vector<WwiseExtractedSound>& results,
                                         const std::string& ownerDirectory,
                                         const std::unordered_set<uint32_t>& visitedMedia,
                                         const std::string& debugName)
    {
        _visitedHierarchies.clear();
        _visitedWemIds.clear();

        for (const uint32_t id : visitedMedia)
            _visitedWemIds.insert(id);

        EventWalk walk;
        walk.Results = &results;
        walk.OwnerDirectory = ownerDirectory;
        walk.DebugName = debugName;
        walk.EventId = eventId;

        TraverseAndSave(walk, eventId);
    }

    void WwiseProvider::TraverseAndSave(EventWalk& walk, uint32_t id)
    {
        for (const Hierarchy* hierarchy : GetHierarchiesById(id))
        {
            if (!_visitedHierarchies.insert(std::make_pair(id, hierarchy)).second)
                continue;

            const auto* data = hierarchy->Data.get();

            if (const auto* soundSfx = dynamic_cast<const C::HierarchySoundSfxVoice*>(data))
            {
                // C# tests `soundSfx.Source is { Plugin.Type: EAkPluginType.Codec }`; Source is a value here,
                // so only the plugin-type half of that pattern survives.
                if (soundSfx->Source.Plugin.Type() == EAkPluginType::Codec)
                    SaveWemSound(walk, soundSfx->Source.SourceId);
                else
                    TraverseAndSave(walk, soundSfx->Source.SourceId);
            }
            else if (const auto* musicRandomSequenceContainer =
                         dynamic_cast<const C::HierarchyMusicRandomSequenceContainer*>(data))
            {
                for (const uint32_t childId : musicRandomSequenceContainer->ChildIds)
                    TraverseAndSave(walk, childId);
            }
            else if (const auto* musicSwitchContainer = dynamic_cast<const C::HierarchyMusicSwitchContainer*>(data))
            {
                for (const uint32_t childId : musicSwitchContainer->ChildIds)
                    TraverseAndSave(walk, childId);
                for (const auto& node : musicSwitchContainer->DecisionTree.Nodes)
                {
                    if (node == nullptr) continue;
                    for (const auto& nodeChild : node->Children)
                    {
                        if (nodeChild != nullptr) TraverseDecisionTreeNode(walk, *nodeChild);
                    }
                }
            }
            else if (const auto* musicTrack = dynamic_cast<const C::HierarchyMusicTrack*>(data))
            {
                for (const auto& playlist : musicTrack->Playlist)
                    SaveWemSound(walk, playlist.SourceId);
            }
            else if (const auto* musicSegment = dynamic_cast<const C::HierarchyMusicSegment*>(data))
            {
                for (const uint32_t childId : musicSegment->ChildIds)
                    TraverseAndSave(walk, childId);
            }
            else if (const auto* randomContainer = dynamic_cast<const C::HierarchyRandomSequenceContainer*>(data))
            {
                for (const uint32_t childId : randomContainer->ChildIds)
                    TraverseAndSave(walk, childId);
            }
            else if (const auto* switchContainer = dynamic_cast<const C::HierarchySwitchContainer*>(data))
            {
                const auto match = std::find_if(walk.SwitchStates.begin(), walk.SwitchStates.end(),
                    [&](const CAkActionSetSwitch& x) { return x.SwitchGroupId == switchContainer->GroupId; });
                if (match != walk.SwitchStates.end())
                {
                    TraverseSwitchContainer(walk, *switchContainer, match->SwitchStateId);
                }
                else if (switchContainer->DefaultSwitch == 0 || walk.SwitchStates.empty())
                {
                    for (const uint32_t childId : switchContainer->ChildIds)
                        TraverseAndSave(walk, childId);
                }
                else
                {
                    TraverseSwitchContainer(walk, *switchContainer, switchContainer->DefaultSwitch);
                }
            }
            else if (const auto* layerContainer = dynamic_cast<const C::HierarchyLayerContainer*>(data))
            {
                for (const uint32_t childId : layerContainer->ChildIds)
                    TraverseAndSave(walk, childId);
            }
            // Skip mixers cause it resolves too many sounds from other events
            //else if (const auto* mixerContainer = dynamic_cast<const C::HierarchyActorMixer*>(data))
            //{
            //    for (const uint32_t childId : mixerContainer->ChildIds)
            //        TraverseAndSave(walk, childId);
            //}
            else if (const auto* fxCustom = dynamic_cast<const C::HierarchyFxCustom*>(data))
            {
                for (const auto& childId : fxCustom->MediaList)
                    SaveWemSound(walk, childId.SourceId);
            }
            else if (const auto* eventContainer = dynamic_cast<const C::HierarchyEvent*>(data))
            {
                const size_t saved = walk.SwitchStates.size();
                for (const uint32_t actionId : eventContainer->EventActionIds)
                {
                    const auto entry = _wwiseHierarchyTables.find(actionId);
                    if (entry == _wwiseHierarchyTables.end()) continue;
                    const auto* eventAction =
                        dynamic_cast<const C::HierarchyEventAction*>(entry->second->Data.get());
                    if (eventAction == nullptr) continue;

                    const auto* setSwitch = eventAction->Get<Objects::Actions::CAkActionSetSwitch>();
                    if (eventAction->EventActionType == EAkActionType::SetSwitch && setSwitch != nullptr)
                    {
                        walk.SwitchStates.push_back(*setSwitch);
                    }
                    else
                    {
                        TraverseAndSave(walk, eventAction->ReferencedId);
                    }
                }

                walk.SwitchStates.resize(saved);
            }
            else
            {
                // C# warns "Unhandled hierarchy type {0}, while traversing through Event {1}".
            }
        }
    }

    void WwiseProvider::TraverseDecisionTreeNode(EventWalk& walk, const AkDecisionTreeNode& node)
    {
        TraverseAndSave(walk, node.AudioNodeId);
        for (const auto& nodeChildTraverse : node.Children)
        {
            if (nodeChildTraverse != nullptr) TraverseDecisionTreeNode(walk, *nodeChildTraverse);
        }
    }

    void WwiseProvider::TraverseSwitchContainer(EventWalk& walk, const HierarchySwitchContainer& switchContainer,
                                                uint32_t stateId)
    {
        for (const auto& state : switchContainer.SwitchPackages)
        {
            // C#'s `x.NodeIds is not null` guard: the port's NodeIds is a vector, never null, and an empty
            // one iterates zero times either way.
            if (state.SwitchId != stateId) continue;
            for (const uint32_t node : state.NodeIds)
                TraverseAndSave(walk, node);
        }
    }

    void WwiseProvider::SaveWemSound(EventWalk& walk, uint32_t wemId)
    {
        if (!_visitedWemIds.insert(wemId).second)
            return;

        std::string fileName = std::to_string(wemId);
        const auto wemGameFile = _looseWemFilesLookup.find(wemId);
        const auto wemData = _wwiseEncodedMedia.find(fileName);
        // C# uses the non-short-circuiting `|` so both lookups always run; with plain finds that is moot,
        // but the "either one hit" condition is the same.
        if (wemGameFile != _looseWemFilesLookup.end() || wemData != _wwiseEncodedMedia.end())
        {
            if (!walk.DebugName.empty() && walk.DebugName != "None")
                fileName = walk.DebugName + " (" + fileName + ")";

            std::string outputPath = PathCombine(walk.OwnerDirectory, fileName);
            if (!outputPath.empty() && outputPath.front() == '/')
                outputPath = outputPath.substr(1);

            std::shared_ptr<FDeferredByteData> data =
                (wemGameFile != _looseWemFilesLookup.end() && wemGameFile->second != nullptr &&
                 wemGameFile->second->IsValid())
                    ? std::static_pointer_cast<FDeferredByteData>(wemGameFile->second)
                    : (wemData != _wwiseEncodedMedia.end() ? wemData->second : nullptr);

            walk.Results->push_back(WwiseExtractedSound{Replace(outputPath, '\\', '/'), "wem", data});
        }
        else
        {
            // C# logs "Failed to load data for '{WemId}' wem file during event resolution".
        }
    }

    int WwiseProvider::LoadExternalWwiseFiles()
    {
        namespace fs = std::filesystem;

        std::error_code ec;
        fs::path searchDirectory(_gameDirectory);
        if (!EqualsIgnoreCase(searchDirectory.filename().string(), "Paks"))
            return 0;

        if (searchDirectory.has_parent_path())
            searchDirectory = searchDirectory.parent_path();

        fs::path wwiseDir;
        for (fs::recursive_directory_iterator it(searchDirectory, fs::directory_options::skip_permission_denied, ec),
             end; it != end && !ec; it.increment(ec))
        {
            if (it->is_directory(ec) && it->path().filename() == "WwiseAudio") { wwiseDir = it->path(); break; }
        }

        if (wwiseDir.empty())
        {
            // C# warns that external Wwise files might not exist.
            return 0;
        }

        std::vector<fs::path> wemFiles;
        std::vector<fs::path> wwiseBankFiles;
        for (fs::recursive_directory_iterator it(wwiseDir, fs::directory_options::skip_permission_denied, ec), end;
             it != end && !ec; it.increment(ec))
        {
            if (!it->is_regular_file(ec)) continue;
            const std::string path = it->path().string();
            if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".wem") == 0) wemFiles.push_back(it->path());
            else if (path.size() >= 4 && (path.compare(path.size() - 4, 4, ".bnk") == 0 ||
                                          path.compare(path.size() - 4, 4, ".pck") == 0))
                wwiseBankFiles.push_back(it->path());
        }

        for (const fs::path& wem : wemFiles)
        {
            const std::string idString = wem.stem().string();
            uint32_t wemId = 0;
            if (TryParseUInt(idString, wemId))
            {
                auto gameFile = std::make_shared<FileProvider::Objects::OsGameFile>(wem, _provider.Versions);
                _looseWemFilesLookup[wemId] = std::make_shared<FGameFileDeferredByteData>(std::move(gameFile));
            }
        }

        int loadedBanks = 0;
        for (const fs::path& bnk : wwiseBankFiles)
        {
            auto gameFile = std::make_shared<FileProvider::Objects::OsGameFile>(bnk, _provider.Versions);
            if (TryLoadAndCacheWwiseFile(gameFile))
                loadedBanks++;
        }
        return loadedBanks;
    }

    void WwiseProvider::BulkInitializeWwise()
    {
        if (_completedWwiseFullBnkInit)
            return;

        int totalLoadedBanks = static_cast<int>(_multiReferenceLibraryCache.size());
        totalLoadedBanks += LoadExternalWwiseFiles();

        std::vector<std::shared_ptr<GameFile>> wwiseFiles;
        _provider.Files.ForEach([&](const std::string&, const std::shared_ptr<GameFile>& file)
        {
            if (ValidWwiseExtensions().count(ToLowerInvariant(file->Extension())) != 0)
                wwiseFiles.push_back(file);
        });
        // We need to prioritize .pck over .bnk (if there's such pair, .bnk might contain only partial audio buffer,
        // full one is stored in .pck). OrderByDescending is stable in LINQ, and std::stable_sort matches that.
        std::stable_sort(wwiseFiles.begin(), wwiseFiles.end(),
            [](const std::shared_ptr<GameFile>& a, const std::shared_ptr<GameFile>& b)
            {
                return EqualsIgnoreCase(a->Extension(), "pck") > EqualsIgnoreCase(b->Extension(), "pck");
            });

        if (wwiseFiles.empty())
        {
            bool initAsset = false;
            _provider.Files.ForEach([&](const std::string&, const std::shared_ptr<GameFile>& file)
            {
                if (initAsset) return;
                if (EqualsIgnoreCase(file->Extension(), "uasset") &&
                    (ContainsIgnoreCase(file->Path(), "Init") || ContainsIgnoreCase(file->Path(), "InitBank")))
                    initAsset = true;
            });

            if (initAsset)
            {
                // TEMP: Init bnk was found, but caching isn't supported yet, prevent exception from throwing
                _completedWwiseFullBnkInit = true;
                return;
            }
        }

        for (const std::shared_ptr<GameFile>& wwiseFile : wwiseFiles)
        {
            const std::string& fullPath = wwiseFile->Path();
            const std::string soundBankName = GetFileNameWithoutExtension(fullPath);
            const std::string extension = ToLowerInvariant(Utils::SubstringAfterLast(fullPath, '.'));
            const bool isWemFile = extension == "wem";

            if (isWemFile)
            {
                uint32_t wemId = 0;
                if (TryParseUInt(soundBankName, wemId) && _looseWemFilesLookup.count(wemId) == 0)
                {
                    _looseWemFilesLookup[wemId] = std::make_shared<FGameFileDeferredByteData>(wwiseFile);
                }

                continue;
            }

            if (!TryLoadAndCacheWwiseFile(wwiseFile))
                continue;

            totalLoadedBanks += 1;
        }

        _completedWwiseFullBnkInit = totalLoadedBanks > 0;
    }

    bool WwiseProvider::TryLoadAndCacheWwiseFile(const std::shared_ptr<GameFile>& gameFile)
    {
        if (gameFile == nullptr) return false;
        const std::optional<std::vector<uint8_t>> data = gameFile->SafeRead();
        if (!data.has_value() || data->empty())
            return false;

        FWwiseArchive reader(gameFile->NameWithoutExtension(), *data);
        try
        {
            auto wwiseReader = std::make_unique<WwiseReader>(reader, WwiseDataSource::FromGameFile(*gameFile));
            WwiseReader* raw = wwiseReader.get();
            _ownedReaders.push_back(std::move(wwiseReader));
            _totalLoadedWwiseSize += raw->LoadedSize;
            _totalWwiseBanksSize += raw->TotalSize;
            CacheWwiseFile(*raw);
            if (raw->Header.SoundBankId != 0) // .pck files don't contain SoundBankId so it's always 0
                _wwiseLoadedSoundBanks.insert(raw->Header.SoundBankId);
        }
        catch (const std::exception&)
        {
            // C# warns "Failed to cache Wwise sound bank file {0}".
            return false;
        }

        return true;
    }

    void WwiseProvider::CacheWwiseFile(WwiseReader& wwiseReader)
    {
        for (const auto& bank : wwiseReader.AKPKBankEntries)
        {
            if (bank != nullptr) CacheWwiseFile(*bank);
        }

        for (const Hierarchy& h : wwiseReader.Hierarchies)
        {
            // Not needed for resolving audio
            if (h.Type == EAKBKHircType::AudioBus || h.Type == EAKBKHircType::ActorMixer)
                continue;
            if (h.Data == nullptr) continue;
            const uint32_t id = h.Data->Id;
            const auto existing = _wwiseHierarchyTables.find(id);
            if (existing != _wwiseHierarchyTables.end())
            {
                auto& duplicates = _wwiseHierarchyDuplicates[id];
                if (duplicates.empty())
                    duplicates.push_back(existing->second);

                duplicates.push_back(&h);
            }
            else
            {
                _wwiseHierarchyTables[id] = &h;
            }
        }

        for (const auto& kv : wwiseReader.WwiseEncodedMedias)
        {
            if (_wwiseEncodedMedia.count(kv.first) == 0)
                _wwiseEncodedMedia[kv.first] = kv.second;
        }

        if (wwiseReader.WemFile != nullptr && wwiseReader.WemFile->IsValid())
            _wwiseEncodedMedia[wwiseReader.Path] = wwiseReader.WemFile; // wwiseReader.Path here needs to be wem file name!
    }

    std::vector<std::string> WwiseProvider::LoadWwisePackagingSettings()
    {
        // C# names the local `engineConfig` but reads DefaultGame; kept.
        const auto& engineConfig = _provider.DefaultGame;

        std::vector<std::string> values;
        engineConfig.EvaluatePropertyValues("/Script/AkAudio.AkSettings", "WwiseStagingDirectory", values);
        const std::string path =
            values.empty() ? std::string()
                           : Utils::SubstringBefore(Utils::SubstringAfter(values.front(), "Path=\""), "\")");
        if (!path.empty())
        {
            // C# maps both separators onto Path.DirectorySeparatorChar and trims a leading one; this port
            // normalises onto '/' to match PathCombine.
            std::string normalised = Replace(path, '\\', '/');
            while (!normalised.empty() && normalised.front() == '/') normalised.erase(normalised.begin());
            _baseWwiseAudioPath = PathCombine(PathCombine(_provider.ProjectName(), "Content"), normalised);
        }
        values.clear();
        engineConfig.EvaluatePropertyValues("/Script/WwisePackaging.WwisePackagingSettings", "AssetLibraries", values);

        std::vector<std::string> result;
        result.reserve(values.size());
        for (const std::string& value : values)
        {
            std::string trimmed = value;
            while (!trimmed.empty() && trimmed.front() == '/') trimmed.erase(trimmed.begin());
            result.push_back(_provider.FixPath(Utils::SubstringBeforeLast(trimmed, '.') + ".uasset"));
        }
        return result;
    }

    void WwiseProvider::LoadMultiReferenceLibrary()
    {
        if (_loadedMultiRefLibrary)
            return;

        _loadedMultiRefLibrary = true;

        const std::vector<std::string> configFiles = LoadWwisePackagingSettings();

        std::vector<std::shared_ptr<GameFile>> assetFiles;
        _provider.Files.ForEach([&](const std::string&, const std::shared_ptr<GameFile>& f)
        {
            const std::string& p = f->Path();
            const std::string suffix = "ReferenceAssetLibrary.uasset";
            const bool endsWith = p.size() >= suffix.size() &&
                EqualsIgnoreCase(p.substr(p.size() - suffix.size()), suffix);
            const bool listed = std::any_of(configFiles.begin(), configFiles.end(),
                [&](const std::string& c) { return _provider.PathComparer.Equals(c, p); });
            if (endsWith || listed) assetFiles.push_back(f);
        });

        for (const auto& assetFile : assetFiles)
        {
            TryLoadMultiReferenceAsset(assetFile);
        }
    }

    void WwiseProvider::TryLoadMultiReferenceAsset(const std::shared_ptr<GameFile>& assetFile)
    {
        if (assetFile == nullptr)
            return;

        try
        {
            Assets::IPackage& package = _provider.LoadPackage(*assetFile);

            // C#'s package.GetExports().OfType<UWwiseAssetLibrary>().FirstOrDefault().
            UWwiseAssetLibrary* wwiseAssetLib = nullptr;
            const auto* summary = package.GetSummary();
            const int exportCount = summary != nullptr ? summary->ExportCount : 0;
            for (int i = 0; i < exportCount && wwiseAssetLib == nullptr; i++)
                wwiseAssetLib = dynamic_cast<UWwiseAssetLibrary*>(package.GetExportObject(i));

            if (wwiseAssetLib == nullptr)
            {
                // C# warns "No UWwiseAssetLibrary found in the package {0}".
                return;
            }

            int64_t loadedSize = 0;
            int64_t totalSize = 0;
            if (wwiseAssetLib->CookedData.has_value())
            {
                for (auto& pf : wwiseAssetLib->CookedData->PackagedFiles)
                {
                    if (pf.BulkData != nullptr && _multiReferenceLibraryCache.count(pf.Hash) == 0)
                    {
                        CacheWwiseFile(*pf.BulkData);
                        _multiReferenceLibraryCache[pf.Hash] = pf.BulkData.get();
                        loadedSize += pf.BulkData->LoadedSize;
                        totalSize += pf.BulkData->TotalSize;
                    }
                }
            }
            _totalLoadedWwiseSize += loadedSize;
            _totalWwiseBanksSize += totalSize;
        }
        catch (const std::exception&)
        {
            // C# logs "Failed to load {Name}".
        }
    }

    std::vector<const Hierarchy*> WwiseProvider::GetHierarchiesById(uint32_t id) const
    {
        // C# yields these lazily; the port materialises the (very short) list so the caller can recurse into
        // TraverseAndSave without holding an iterator over a map it may mutate.
        std::vector<const Hierarchy*> result;
        if (const auto primary = _wwiseHierarchyTables.find(id); primary != _wwiseHierarchyTables.end())
        {
            result.push_back(primary->second);
        }

        if (const auto duplicates = _wwiseHierarchyDuplicates.find(id); duplicates != _wwiseHierarchyDuplicates.end())
        {
            for (const Hierarchy* duplicate : duplicates->second)
            {
                result.push_back(duplicate);
            }
        }
        return result;
    }

    std::string WwiseProvider::GetOwnerDirectory(const Assets::Exports::UObject& obj) const
    {
        const std::string ownerName = obj.Owner != nullptr ? obj.Owner->GetName() : _baseWwiseAudioPath;
        const std::string path = _provider.FixPath(ownerName);
        const std::string directory = GetDirectoryName(path);
        return directory.empty() ? _baseWwiseAudioPath : directory;
    }
}
