// Ported from CUE4Parse/UE4/Versions/VersionContainer.cs
// The per-provider version state: Game / Ver / Platform plus the two per-game lookup tables the readers
// branch on — Options (named booleans) and MapStructTypes (per-property-name key/value struct types).
//
// Deliberate differences from C#:
//   * C#'s `Game` setter leaves `_ver` untouched, so assigning Game after construction leaves Ver pinned to
//     the previous game's value unless Ver is reassigned too. SetGame here re-derives Ver whenever it was
//     not explicitly pinned (bExplicitVer == false), which is what every C# caller arranges by hand.
//   * The override tables are plain maps rather than nullable ones: C# only ever iterates them, so a null
//     override and an empty override are indistinguishable.
//   * MapStructTypes' value is a pair of strings rather than KeyValuePair<string, string?>. C# only tests
//     the entries with string.IsNullOrEmpty, so an empty string stands in for null.
//   * C#'s ICloneable.Clone() is dropped — the implicit copy constructor already produces the same object
//     (including the override tables, which Clone() passes through).
//   * Options entries whose consumers are not ported yet (the mesh/shader/sound families) are still
//     populated, so the table is complete even where nothing reads it.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "EGame.h"
#include "FPackageFileVersion.h"
#include "../Assets/Exports/Texture/ETexturePlatform.h"

// Forward-declared so VersionContainer can hold an optional custom-version table without pulling in
// FCustomVersionContainer.h (which depends on FArchive, which includes this header — a cycle).
namespace CUE4Parse::UE4::Objects::Core::Serialization { class FCustomVersionContainer; }

namespace CUE4Parse::UE4::Versions
{
    using CUE4Parse::UE4::Assets::Exports::Texture::ETexturePlatform;

    class VersionContainer
    {
    public:
        // C#'s KeyValuePair<string, string?> value of MapStructTypes: the key struct type and the value
        // struct type of a TMap property, either of which may be empty meaning "leave as-is".
        using FMapStructTypes = std::pair<std::string, std::string>;

        explicit VersionContainer(EGame game = GAME_UE4_LATEST,
                                  ETexturePlatform platform = ETexturePlatform::DesktopMobile,
                                  FPackageFileVersion ver = {},
                                  std::map<std::string, bool> optionOverrides = {},
                                  std::map<std::string, FMapStructTypes> mapStructTypesOverrides = {})
            : _optionOverrides(std::move(optionOverrides))
            , _mapStructTypesOverrides(std::move(mapStructTypesOverrides))
        {
            _platform = platform;
            SetGame(game); // sets _game and, when ver is not explicit, derives Ver from the game
            SetVer(ver);   // re-inits the tables so the Ver-dependent options see the final Ver
        }

        EGame Game() const { return _game; }
        void SetGame(EGame value)
        {
            _game = value;
            // When the version was not explicitly pinned, keep Ver tracking the game's default version.
            if (!bExplicitVer) _ver = GetVersion(_game);
            InitOptions();
            InitMapStructTypes();
        }

        FPackageFileVersion Ver() const { return _ver; }
        void SetVer(FPackageFileVersion value)
        {
            bExplicitVer = value.FileVersionUE3 != 0 || value.FileVersionUE4 != 0 || value.FileVersionUE5 != 0;
            _ver = bExplicitVer ? value : GetVersion(_game);
            // C# re-runs InitOptions from the Platform setter after Ver is assigned; the port re-runs it
            // here so "StaticMesh.HasNavCollision" is right no matter which setter ran last.
            InitOptions();
        }

        ETexturePlatform Platform() const { return _platform; }
        void SetPlatform(ETexturePlatform value)
        {
            _platform = value;
            InitOptions();
            InitMapStructTypes();
        }

        bool bExplicitVer = false;

        // Optional custom-version table (null until a package summary supplies one). Mirrors the C#
        // nullable FCustomVersionContainer? field. Held by shared_ptr so the fwd declaration suffices.
        std::shared_ptr<Objects::Core::Serialization::FCustomVersionContainer> CustomVersions;

        std::map<std::string, bool> Options;
        std::map<std::string, FMapStructTypes> MapStructTypes;

        // C#'s `this[string optionKey]` getter. Like the C# Dictionary indexer this throws when the option
        // does not exist rather than defaulting to false — a missing key means a typo, not "off".
        bool operator[](const std::string& optionKey) const;
        // C#'s indexer setter.
        void SetOption(const std::string& optionKey, bool value) { Options[optionKey] = value; }

    private:
        void InitOptions();
        void InitMapStructTypes();
        // C#'s OverrideUseAudioStreaming: the games that ship 4.25+ sound waves without audio streaming.
        bool OverrideUseAudioStreaming() const;

        EGame _game = GAME_UE4_LATEST;
        FPackageFileVersion _ver{};
        ETexturePlatform _platform = ETexturePlatform::DesktopMobile;

        std::map<std::string, bool> _optionOverrides;
        std::map<std::string, FMapStructTypes> _mapStructTypesOverrides;
    };
}
