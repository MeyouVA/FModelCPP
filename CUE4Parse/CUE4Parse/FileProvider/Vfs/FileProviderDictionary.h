// Ported from CUE4Parse/FileProvider/Vfs/FileProviderDictionary.cs
// The provider's view over every mounted container: an ordered bag of (readOrder, files) indices, searched
// highest readOrder first so patch paks shadow base paks.
//
// Deliberate differences from C#:
//   * C#'s ConcurrentBag holds *references* to each reader's live dictionary; nothing pins those in C++, so
//     AddFiles copies the map instead (cheap — GameFileMap holds shared_ptr, so entries stay shared with the
//     reader). The concurrency wrappers are dropped with the threading layer.
//   * Lookup order: C# sorts the bag OrderByDescending(readOrder); for equal readOrders the bag's enumeration
//     order is unspecified. Here the indices are kept stably sorted descending, so equal readOrders resolve
//     to the earliest-added index — a deterministic pick inside behaviour C# leaves undefined.
//   * The Keys/Values enumerable wrappers become the ForEach visitor, which walks entries in the same
//     descending-readOrder order as C#'s GetEnumerator.
//   * The indexer this[path] becomes At(path), throwing std::out_of_range for KeyNotFoundException.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../Objects/GameFile.h"
#include "../../UE4/IO/Objects/FPackageId.h"
#include "../../UE4/VirtualFileSystem/IVfsReader.h"

namespace CUE4Parse::FileProvider::Vfs
{
    using UE4::VirtualFileSystem::GameFileMap;
    using Objects::GameFile;

    class FileProviderDictionary
    {
    public:
        // C#'s ById: UE packages from IO Store containers, by their FPackageId (only the normal package,
        // not the .o optional variant — they share the id).
        const std::map<UE4::IO::Objects::FPackageId, std::shared_ptr<GameFile>>& ById() const { return _byId; }

        // C#'s FindPayloads: the sibling .uexp/.ubulk/.uptnl files for a package, preferring the package's
        // own archive so patched archives stay self-consistent. With cookedIndexLookup an IO Store
        // package's payloads are found through its reader's chunk ids instead.
        void FindPayloads(const GameFile& file,
                          std::shared_ptr<GameFile>& uexp,
                          std::vector<std::shared_ptr<GameFile>>& ubulks,
                          std::vector<std::shared_ptr<GameFile>>& uptnls,
                          bool cookedIndexLookup = false) const;

        // Registers a mounted container's files under its read order. Copies the map (see header comment).
        void AddFiles(GameFileMap newFiles, int64_t readOrder = 0);

        void Clear() { _indicesBag.clear(); _byId.clear(); }

        bool ContainsKey(const std::string& key) const;

        // Highest readOrder wins, like C#'s OrderByDescending scan.
        bool TryGetValue(const std::string& key, std::shared_ptr<GameFile>& value) const;

        // Every match across all indices, best first.
        bool TryGetValues(const std::string& key, std::vector<std::shared_ptr<GameFile>>& values) const;

        // C#'s this[path]; throws when the path is in no index.
        std::shared_ptr<GameFile> At(const std::string& path) const
        {
            std::shared_ptr<GameFile> value;
            if (!TryGetValue(path, value)) throw std::out_of_range("There is no game file with the path \"" + path + "\"");
            return value;
        }

        int Count() const
        {
            size_t n = 0;
            for (const auto& index : _indicesBag) n += index.second.size();
            return static_cast<int>(n);
        }

        // Visits every (path, file) pair in C#'s enumeration order (descending readOrder). The visitor
        // returns void; shadowed duplicates across indices are visited too, exactly as in C#.
        template <typename Fn>
        void ForEach(Fn&& visit) const
        {
            for (const auto& index : _indicesBag)
                for (const auto& [path, file] : index.second)
                    visit(path, file);
        }

        // C#'s Keys.FirstOrDefault(predicate): the first key (in the same enumeration order as ForEach)
        // matching the predicate, or null. The pointer is valid until the dictionary is next mutated.
        template <typename Pred>
        const std::string* FirstKey(Pred&& pred) const
        {
            for (const auto& index : _indicesBag)
                for (const auto& [path, file] : index.second)
                    if (pred(path)) return &path;
            return nullptr;
        }

    private:
        // Kept stably sorted descending by readOrder (see header comment).
        std::vector<std::pair<int64_t, GameFileMap>> _indicesBag;
        std::map<UE4::IO::Objects::FPackageId, std::shared_ptr<GameFile>> _byId;
    };
}
