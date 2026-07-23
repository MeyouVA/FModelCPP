// Ported from CUE4Parse/MappingsProvider/Usmap/UsmapTypeMappingsProvider.cs and
// FileUsmapTypeMappingsProvider.cs.
// The usmap-backed mappings providers: the abstract base loads a path/bytes through UsmapParser; the file
// provider remembers its path (+ comparer) so Reload() re-parses the same file.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "UsmapParser.h"
#include "../ITypeMappingsProvider.h"
#include "../../UE4/Readers/FByteArchive.h"

namespace CUE4Parse::MappingsProvider::Usmap
{
    class UsmapTypeMappingsProvider : public AbstractTypeMappingsProvider
    {
    public:
        const TypeMappings* MappingsForGame() const override { return _mappingsForGame.get(); }

        void Load(const std::string& path, std::optional<Utils::StringComparer> comparer = std::nullopt) override
        {
            UsmapParser usmap(path, comparer);
            _mappingsForGame = usmap.Mappings;
        }

        void Load(const std::vector<uint8_t>& bytes, std::optional<Utils::StringComparer> comparer = std::nullopt) override
        {
            UE4::Readers::FByteArchive archive("An unnamed usmap", bytes);
            UsmapParser usmap(archive, comparer);
            _mappingsForGame = usmap.Mappings;
        }

    protected:
        // C#'s `MappingsForGame { get; protected set; } = new()` — starts as an empty TypeMappings.
        std::shared_ptr<TypeMappings> _mappingsForGame = std::make_shared<TypeMappings>();
    };

    class FileUsmapTypeMappingsProvider : public UsmapTypeMappingsProvider
    {
    public:
        explicit FileUsmapTypeMappingsProvider(std::string path,
                                               std::optional<Utils::StringComparer> comparer = std::nullopt)
            : _stringComparer(comparer), _path(std::move(path))
        {
            Load(_path, _stringComparer);
        }

        std::string FileName() const
        {
            const auto slash = _path.find_last_of("/\\");
            return slash == std::string::npos ? _path : _path.substr(slash + 1);
        }

        void Reload() override
        {
            Load(_path, _stringComparer);
        }

    private:
        std::optional<Utils::StringComparer> _stringComparer;
        std::string _path;
    };
}
