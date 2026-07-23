#include "FileProviderDictionary.h"

#include <algorithm>

#include "../../UE4/IO/IoStoreReader.h"
#include "../../UE4/IO/Objects/FIoStoreEntry.h"
#include "../../UE4/VirtualFileSystem/VfsEntry.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::FileProvider::Vfs
{
    using UE4::IO::Objects::FIoStoreEntry;
    using UE4::VirtualFileSystem::VfsEntry;

    namespace
    {
        bool EndsWith(const std::string& s, const std::string& suffix)
        {
            return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
    }

    void FileProviderDictionary::FindPayloads(const GameFile& file,
                                              std::shared_ptr<GameFile>& uexp,
                                              std::vector<std::shared_ptr<GameFile>>& ubulks,
                                              std::vector<std::shared_ptr<GameFile>>& uptnls,
                                              bool cookedIndexLookup) const
    {
        uexp = nullptr;
        ubulks.clear();
        uptnls.clear();
        if (!file.IsUePackage()) return;

        const std::string& path = file.PathWithoutExtension();

        if (const auto* ioEntry = dynamic_cast<const FIoStoreEntry*>(&file);
            cookedIndexLookup && ioEntry != nullptr && ioEntry->IsUePackage())
        {
            // dedicated to FBulkDataCookedIndex payloads but should work fine for anything coming from
            // IoStore; hitting IoStore Files like that is quite slow, but it's the only way to get the
            // correct payloads
            for (const auto& [payloadPath, payload] : ioEntry->GetIoStoreReader().Files())
            {
                const auto* payloadIo = dynamic_cast<const FIoStoreEntry*>(payload.get());
                if (!payload->IsUePackagePayload() || payloadIo == nullptr ||
                    payloadIo->ChunkId().ChunkId != ioEntry->ChunkId().ChunkId)
                    continue;

                if (payload->Extension() == "ubulk") ubulks.push_back(payload);
                else if (payload->Extension() == "uptnl") uptnls.push_back(payload);
            }
        }
        else if (const auto* entry = dynamic_cast<const VfsEntry*>(&file); entry != nullptr && entry->Vfs != nullptr)
        {
            // The file comes from a specific archive: prefer payloads from that same archive, which is what
            // keeps patched archives self-consistent.
            const auto& vfsFiles = entry->Vfs->Files();
            if (const auto it = vfsFiles.find(path + ".uexp"); it != vfsFiles.end())
                uexp = it->second;
            if (const auto it = vfsFiles.find(path + ".ubulk"); it != vfsFiles.end())
                ubulks.push_back(it->second);
        }

        if (uexp == nullptr) TryGetValue(path + ".uexp", uexp);
        if (ubulks.empty())
        {
            std::shared_ptr<GameFile> ubulk;
            if (TryGetValue(path + ".ubulk", ubulk)) ubulks.push_back(std::move(ubulk));
        }
        if (uptnls.empty())
        {
            std::shared_ptr<GameFile> uptnl;
            if (TryGetValue(path + ".uptnl", uptnl)) uptnls.push_back(std::move(uptnl));
        }
    }

    void FileProviderDictionary::AddFiles(GameFileMap newFiles, int64_t readOrder)
    {
        for (const auto& [path, file] : newFiles)
        {
            // packages, their optional variant and their respective payloads share the same id
            // only load the normal package in this dict for later use by IoPackage.ImportedPackages
            const auto* ioEntry = dynamic_cast<const FIoStoreEntry*>(file.get());
            if (ioEntry != nullptr && ioEntry->IsUePackage() && !EndsWith(file->NameWithoutExtension(), ".o"))
            {
                _byId[ioEntry->ChunkId().AsPackageId()] = file;
            }
        }

        // Stable descending insert: after everything with a strictly greater or equal readOrder, so equal
        // read orders keep their registration order (see header comment).
        const auto pos = std::find_if(_indicesBag.begin(), _indicesBag.end(),
                                      [readOrder](const auto& index) { return index.first < readOrder; });
        _indicesBag.insert(pos, {readOrder, std::move(newFiles)});
    }

    bool FileProviderDictionary::ContainsKey(const std::string& key) const
    {
        for (const auto& index : _indicesBag)
            if (index.second.find(key) != index.second.end())
                return true;
        return false;
    }

    bool FileProviderDictionary::TryGetValue(const std::string& key, std::shared_ptr<GameFile>& value) const
    {
        for (const auto& index : _indicesBag)
        {
            if (const auto it = index.second.find(key); it != index.second.end())
            {
                value = it->second;
                return true;
            }
        }
        value = nullptr;
        return false;
    }

    bool FileProviderDictionary::TryGetValues(const std::string& key, std::vector<std::shared_ptr<GameFile>>& values) const
    {
        values.clear();
        for (const auto& index : _indicesBag)
            if (const auto it = index.second.find(key); it != index.second.end())
                values.push_back(it->second);
        return !values.empty();
    }
}
