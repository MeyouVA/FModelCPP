#include "AbstractVfsFileProvider.h"

#include <algorithm>
#include <stdexcept>

#include "../Objects/OsGameFile.h"
#include "../../UE4/Assets/IPackage.h"
#include "../../UE4/Exceptions/InvalidAesKeyException.h"
#include "../../UE4/IO/Objects/FIoStoreEntry.h"
#include "../../UE4/Pak/PakFileReader.h"
#include "../../UE4/Readers/FStreamArchive.h"
#include "../../UE4/Versions/EGame.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::FileProvider::Vfs
{
    using UE4::Pak::PakFileReader;
    using UE4::Versions::GAME_FragPunk;
    using UE4::Versions::GAME_LordOfMysteries;

    namespace
    {
        std::string UpperExtension(const std::string& name)
        {
            std::string ext = Utils::SubstringAfterLast(name, '.');
            for (auto& c : ext) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
            return ext;
        }
    }

    AbstractVfsFileProvider::AbstractVfsFileProvider(UE4::Versions::VersionContainer versions,
                                                     Utils::StringComparer pathComparer)
        : AbstractFileProvider(std::move(versions), pathComparer)
    {
        // C# wires ~30 per-game GameTypes decryptors into CustomEncryption here; none are ported, so it
        // stays null (see the header comment for why that is an explicit gap rather than wrong bytes).
    }

    void AbstractVfsFileProvider::RegisterVfs(const std::string& file)
    {
        try
        {
            RegisterVfs(std::make_shared<UE4::Readers::FRandomAccessFileStreamArchive>(file, Versions));
        }
        catch (const std::exception&)
        {
            // C# logs a warning here; the port has no logging layer.
        }
    }

    void AbstractVfsFileProvider::RegisterVfs(std::shared_ptr<UE4::Readers::FArchive> archive,
                                              UE4::IO::OpenContainerStreamFunc openContainerStreamFunc)
    {
        try
        {
            archive->Versions = Versions;

            const std::string ext = UpperExtension(archive->Name());
            std::shared_ptr<AbstractAesVfsReader> reader;
            if (ext == "PAK" || (ext == "UPAK" && archive->Game() == GAME_LordOfMysteries))
            {
                reader = std::make_shared<PakFileReader>(std::move(archive));
            }
            else if (ext == "UTOC")
            {
                if (openContainerStreamFunc == nullptr)
                {
                    const auto versions = Versions;
                    openContainerStreamFunc = [versions](const std::string& path)
                    {
                        return std::make_shared<UE4::Readers::FRandomAccessFileStreamArchive>(path, versions);
                    };
                }
                reader = std::make_shared<UE4::IO::IoStoreReader>(std::move(archive), std::move(openContainerStreamFunc));
            }
            else
            {
                // C#'s UONDEMANDTOC branch needs the on-demand downloader (unported); everything else is
                // skipped there too.
                return;
            }
            PostLoadReader(std::move(reader));
        }
        catch (const std::exception&)
        {
            // C# logs a warning here; the port has no logging layer.
        }
    }

    void AbstractVfsFileProvider::PostLoadReader(std::shared_ptr<AbstractAesVfsReader> reader, bool isConcurrent)
    {
        if (reader->IsEncrypted())
            _requiredKeys.insert(reader->EncryptionKeyGuid());

        _unloadedVfs.push_back(reader);
        reader->SetConcurrent(isConcurrent);
        // no custom encryption for MarvelRivals IoStore (as in C#)
        if (!(reader->Game() == UE4::Versions::GAME_MarvelRivals &&
              dynamic_cast<UE4::IO::IoStoreReader*>(reader.get()) != nullptr))
        {
            reader->CustomEncryption() = CustomEncryption;
        }

        if (VfsRegistered) VfsRegistered(*reader, static_cast<int>(_unloadedVfs.size()));
    }

    int AbstractVfsFileProvider::Mount()
    {
        int countNewMounts = 0;
        // Iterate over a copy: a successful mount moves the reader between the two vectors.
        const std::vector<std::shared_ptr<AbstractAesVfsReader>> unloaded = _unloadedVfs;
        for (const auto& reader : unloaded)
        {
            VerifyGlobalData(*reader);

            if ((reader->IsEncrypted() && CustomEncryption == nullptr) || !reader->HasDirectoryIndex())
                continue;

            try
            {
                reader->MountTo(Files, PathComparer, VfsMounted);
                _unloadedVfs.erase(std::remove(_unloadedVfs.begin(), _unloadedVfs.end(), reader), _unloadedVfs.end());
                _mountedVfs.push_back(reader);
                ++countNewMounts;
            }
            catch (const UE4::Exceptions::InvalidAesKeyException&)
            {
                // Ignore this
            }
            catch (const std::exception&)
            {
                // C# logs "Uncaught exception while loading file ..." here; the port has no logging layer.
            }
        }
        return countNewMounts;
    }

    int AbstractVfsFileProvider::SubmitKeys(const std::vector<std::pair<FGuid, std::shared_ptr<FAesKey>>>& keys)
    {
        int countNewMounts = 0;
        std::vector<std::shared_ptr<AbstractAesVfsReader>> completed;
        for (const auto& [guid, key] : keys)
        {
            const std::vector<std::shared_ptr<AbstractAesVfsReader>> unloaded = _unloadedVfs;
            for (const auto& reader : unloaded)
            {
                if (!(reader->EncryptionKeyGuid() == guid)) continue;

                if (reader->Game() == GAME_FragPunk && Utils::Contains(reader->Name(), "global"))
                    reader->AesKey() = key;
                VerifyGlobalData(*reader);

                if (!reader->HasDirectoryIndex())
                    continue;

                try
                {
                    reader->MountTo(Files, PathComparer, key, VfsMounted);
                    _unloadedVfs.erase(std::remove(_unloadedVfs.begin(), _unloadedVfs.end(), reader), _unloadedVfs.end());
                    _mountedVfs.push_back(reader);
                    ++countNewMounts;
                    completed.push_back(reader);
                }
                catch (const UE4::Exceptions::InvalidAesKeyException&)
                {
                    // Ignore this
                }
                catch (const std::exception&)
                {
                    // C# logs "Uncaught exception while loading pak file ..." here; no logging layer.
                }
            }
        }

        for (const auto& it : completed)
        {
            const auto& key = it->AesKey();
            if (key == nullptr) continue;
            _requiredKeys.erase(it->EncryptionKeyGuid());
            _keys.emplace(it->EncryptionKeyGuid(), key); // TryAdd: emplace keeps an existing entry
        }

        return countNewMounts;
    }

    AbstractAesVfsReader& AbstractVfsFileProvider::GetArchive(const std::string& archiveName,
                                                              const Utils::StringComparer& comparison)
    {
        const auto matches = [&](const std::shared_ptr<AbstractAesVfsReader>& x)
        {
            return comparison.Equals(x->Name(), archiveName);
        };
        if (const auto it = std::find_if(_mountedVfs.begin(), _mountedVfs.end(), matches); it != _mountedVfs.end())
            return **it;
        if (const auto it = std::find_if(_unloadedVfs.begin(), _unloadedVfs.end(), matches); it != _unloadedVfs.end())
            return **it;
        throw std::out_of_range("There is no archive file with the name \"" + archiveName + "\"");
    }

    IAesVfsReader* AbstractVfsFileProvider::TryGetArchive(const std::string& archiveName,
                                                          const Utils::StringComparer& comparison)
    {
        try
        {
            return &GetArchive(archiveName, comparison);
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    std::shared_ptr<Objects::GameFile> AbstractVfsFileProvider::GetGameFile(const std::string& path,
                                                                            const IAesVfsReader& archive) const
    {
        auto file = TryGetGameFile(path, archive.Files());
        if (file == nullptr)
            throw std::out_of_range("There is no game file with the path \"" + path + "\" in \"" + archive.Name() + "\"");
        return file;
    }

    std::shared_ptr<Objects::GameFile> AbstractVfsFileProvider::TryGetGameFile(const std::string& path,
                                                                               const std::string& archiveName,
                                                                               const Utils::StringComparer& comparison)
    {
        try
        {
            return GetGameFile(path, GetArchive(archiveName, comparison));
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    void AbstractVfsFileProvider::VerifyGlobalData(AbstractAesVfsReader& reader)
    {
        if (_globalData != nullptr) return;
        auto* ioStoreReader = dynamic_cast<UE4::IO::IoStoreReader*>(&reader);
        if (ioStoreReader == nullptr) return;

        const auto& name = ioStoreReader->Name();
        const auto ignoreCase = Utils::StringComparer::OrdinalIgnoreCase();
        if (ignoreCase.Equals(name, "global.utoc") || ignoreCase.Equals(name, "global_console_win.utoc"))
        {
            // C# lets a failure here propagate out of Mount/SubmitKeys; both call sites already swallow.
            _globalData = std::make_unique<UE4::IO::IoGlobalData>(*ioStoreReader);
        }
    }

    UE4::Assets::IPackage& AbstractVfsFileProvider::LoadPackage(const UE4::IO::Objects::FPackageId& id)
    {
        const auto& byId = Files.ById();
        const auto it = byId.find(id);
        if (it == byId.end())
            throw std::out_of_range("There is no package with the id " + id.ToString());
        return AbstractFileProvider::LoadPackage(*it->second);
    }

    UE4::Assets::IPackage* AbstractVfsFileProvider::TryLoadPackage(const UE4::IO::Objects::FPackageId& id)
    {
        const auto& byId = Files.ById();
        const auto it = byId.find(id);
        return it != byId.end() ? AbstractFileProvider::TryLoadPackage(*it->second) : nullptr;
    }

    const UE4::IO::Objects::FFilePackageStoreEntry* AbstractVfsFileProvider::TryFindStoreEntry(
        const UE4::IO::Objects::FPackageId& packageId) const
    {
        for (const auto& reader : _mountedVfs)
        {
            auto* ioReader = dynamic_cast<UE4::IO::IoStoreReader*>(reader.get());
            if (ioReader == nullptr) continue;
            auto* header = ioReader->ContainerHeader();
            if (header == nullptr || header->StoreEntries.empty()) continue;

            for (size_t i = 0; i < header->PackageIds.size(); i++)
            {
                if (!(header->PackageIds[i] == packageId)) continue;
                if (i >= header->StoreEntries.size()) break;
                return &header->StoreEntries[i];
            }
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Objects::GameFile>> AbstractVfsFileProvider::ScanForPackageRefs(Objects::GameFile& asset)
    {
        std::vector<std::shared_ptr<Objects::GameFile>> refList;
        if (dynamic_cast<UE4::IO::Objects::FIoStoreEntry*>(&asset) == nullptr || !asset.IsUePackage())
            return refList;

        const UE4::Assets::IPackage& package = AbstractFileProvider::LoadPackage(asset);
        const auto id = UE4::IO::Objects::FPackageId::FromName(package.GetName());
        for (const auto& reader : _mountedVfs)
        {
            auto* ioReader = dynamic_cast<UE4::IO::IoStoreReader*>(reader.get());
            if (ioReader == nullptr) continue;
            auto* header = ioReader->ContainerHeader();
            if (header == nullptr || header->StoreEntries.empty()) continue;

            for (size_t i = 0; i < header->StoreEntries.size() && i < header->PackageIds.size(); i++)
            {
                const auto& imported = header->StoreEntries[i].ImportedPackages;
                if (std::find(imported.begin(), imported.end(), id) == imported.end()) continue;
                const auto it = ioReader->PackageIdIndex.find(header->PackageIds[i]);
                if (it != ioReader->PackageIdIndex.end()) refList.push_back(it->second);
            }
        }
        return refList;
    }

    void AbstractVfsFileProvider::PostMount()
    {
        const bool workingAes = LoadIniConfigs();
        if (workingAes || !DefaultGame.EncryptionKeyGuid.has_value()) return;

        // C# groups the suspect readers by key guid and drops the group whose guid matches DefaultGame's.
        // Only readers that mounted *without* being encrypted themselves yet hold encrypted files can be
        // wrong in this way.
        const FGuid& guid = *DefaultGame.EncryptionKeyGuid;
        std::vector<std::shared_ptr<AbstractAesVfsReader>> stillMounted;
        bool unmountedAny = false;
        for (const auto& reader : _mountedVfs)
        {
            if (reader->IsEncrypted() || reader->EncryptedFileCount() <= 0 ||
                !(reader->EncryptionKeyGuid() == guid))
            {
                stillMounted.push_back(reader);
                continue;
            }

            _unloadedVfs.push_back(reader);
            unmountedAny = true;
            if (VfsUnmounted) VfsUnmounted(*reader, static_cast<int>(_unloadedVfs.size()));
        }

        if (!unmountedAny) return;

        _mountedVfs = std::move(stillMounted);
        _keys.erase(guid);
        _requiredKeys.insert(guid);
    }

    void AbstractVfsFileProvider::UnloadAllVfs()
    {
        Files.Clear();
        for (const auto& reader : _mountedVfs)
        {
            _keys.erase(reader->EncryptionKeyGuid());
            _requiredKeys.insert(reader->EncryptionKeyGuid());
            _unloadedVfs.push_back(reader);
            if (VfsUnmounted) VfsUnmounted(*reader, static_cast<int>(_unloadedVfs.size()));
        }
        _mountedVfs.clear();
    }

    void AbstractVfsFileProvider::UnloadNonStreamedVfs()
    {
        // C# also keeps StreamedGameFile entries; that type is unported, so only loose OS files survive.
        UE4::VirtualFileSystem::GameFileMap onDemandFiles{PathComparer};
        Files.ForEach([&onDemandFiles](const std::string& path, const std::shared_ptr<Objects::GameFile>& vfs)
        {
            if (dynamic_cast<Objects::OsGameFile*>(vfs.get()) != nullptr)
                onDemandFiles[path] = vfs;
        });

        UnloadAllVfs();
        Files.AddFiles(std::move(onDemandFiles));
    }

    void AbstractVfsFileProvider::Dispose()
    {
        AbstractFileProvider::Dispose();

        _unloadedVfs.clear();
        _mountedVfs.clear();
        _keys.clear();
        _requiredKeys.clear();
        _globalData.reset();
    }
}
