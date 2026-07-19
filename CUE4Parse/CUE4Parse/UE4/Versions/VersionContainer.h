// Ported from CUE4Parse/UE4/Versions/VersionContainer.cs (minimal core: Game / Ver / Platform)
// NOTE: The C# VersionContainer also drives per-game Options and MapStructTypes tables via InitOptions()/
// InitMapStructTypes(). Those tables belong to the asset/property layer and are deferred. TODO: port them.
#pragma once

#include <memory>

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
        explicit VersionContainer(EGame game = GAME_UE4_LATEST,
                                  ETexturePlatform platform = ETexturePlatform::DesktopMobile,
                                  FPackageFileVersion ver = {})
        {
            _platform = platform;
            SetGame(game); // sets _game and, when ver is not explicit, derives Ver from the game
            SetVer(ver);
        }

        EGame Game() const { return _game; }
        void SetGame(EGame value)
        {
            _game = value;
            // When the version was not explicitly pinned, keep Ver tracking the game's default version.
            if (!bExplicitVer) _ver = GetVersion(_game);
        }

        FPackageFileVersion Ver() const { return _ver; }
        void SetVer(FPackageFileVersion value)
        {
            bExplicitVer = value.FileVersionUE3 != 0 || value.FileVersionUE4 != 0 || value.FileVersionUE5 != 0;
            _ver = bExplicitVer ? value : GetVersion(_game);
        }

        ETexturePlatform Platform() const { return _platform; }
        void SetPlatform(ETexturePlatform value) { _platform = value; }

        bool bExplicitVer = false;

        // Optional custom-version table (null until a package summary supplies one). Mirrors the C#
        // nullable FCustomVersionContainer? field. Held by shared_ptr so the fwd declaration suffices.
        std::shared_ptr<Objects::Core::Serialization::FCustomVersionContainer> CustomVersions;

    private:
        EGame _game = GAME_UE4_LATEST;
        FPackageFileVersion _ver{};
        ETexturePlatform _platform = ETexturePlatform::DesktopMobile;
    };
}
