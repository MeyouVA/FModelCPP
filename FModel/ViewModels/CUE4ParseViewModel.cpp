// Ported from FModel/ViewModels/CUE4ParseViewModel.cs — see the header for what this slice covers.
#include "CUE4ParseViewModel.h"

#include <filesystem>

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

#include "FileProvider/DefaultFileProvider.h"
#include "MappingsProvider/Usmap/UsmapTypeMappingsProvider.h"
#include "UE4/Objects/Core/Serialization/FCustomVersionContainer.h"
#include "UE4/Versions/VersionContainer.h"

#include "../Constants.h"
#include "../Settings/DirectorySettings.h"
#include "../Settings/EndpointSettings.h"
#include "../Settings/UserSettings.h"
#include "../Settings/VersioningSettings.h"

namespace FModel::ViewModels
{
    using CUE4Parse::Encryption::Aes::FAesKey;
    using CUE4Parse::FileProvider::DefaultFileProvider;
    using CUE4Parse::FileProvider::SearchOption;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Versions::EGame;
    using CUE4Parse::UE4::Versions::VersionContainer;
    using CUE4Parse::Utils::StringComparer;

    namespace
    {
        // C#'s `gameDirectory.SubstringBeforeLast(... "\\pak" : "\\Content").SubstringAfterLast("\\")` — the
        // folder that names the project. Backslashes are what upstream matches on, so both separators are
        // accepted here rather than normalising, which would change which games hit which arm.
        QString projectFolderName(const QString& gameDirectory)
        {
            const QString marker = gameDirectory.contains(QStringLiteral("eFootball"))
                ? QStringLiteral("\\pak")
                : QStringLiteral("\\Content");

            QString head = gameDirectory;
            const int markerAt = head.lastIndexOf(marker, -1, Qt::CaseInsensitive);
            if (markerAt >= 0)
                head = head.left(markerAt);

            const int slash = qMax(head.lastIndexOf(QLatin1Char('\\')), head.lastIndexOf(QLatin1Char('/')));
            return slash >= 0 ? head.mid(slash + 1) : head;
        }

        QString localAppData()
        {
            return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        }

        QString commonAppData()
        {
            // Environment.SpecialFolder.CommonApplicationData == %PROGRAMDATA%.
            const QString fromEnv = qEnvironmentVariable("ProgramData");
            return fromEnv.isEmpty() ? QStringLiteral("C:/ProgramData") : QDir::fromNativeSeparators(fromEnv);
        }

        std::filesystem::path toPath(const QString& s)
        {
            return std::filesystem::path(s.toStdWString());
        }
    }

    CUE4ParseViewModel::CUE4ParseViewModel(ThreadWorkerViewModel* worker, QObject* parent)
        : ViewModel(parent), _worker(worker)
    {
        const auto* settings = Settings::UserSettings::Default();
        const Settings::DirectorySettings* currentDir = settings->currentDir();

        const EGame game = currentDir != nullptr ? currentDir->ueVersion() : CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        const QString gameDirectory = currentDir != nullptr ? currentDir->gameDirectory() : QString();

        // C#: new VersionContainer(game, platform, customVersions, optionOverrides, mapStructTypesOverrides).
        // The C++ ctor takes `ver` in third position (C# has no such parameter — it defaults there too), so
        // the two override maps are converted and passed by position.
        std::map<std::string, bool> options;
        std::map<std::string, VersionContainer::FMapStructTypes> mapStructTypes;
        if (currentDir != nullptr && currentDir->versioning() != nullptr)
        {
            const Settings::VersioningSettings* versioning = currentDir->versioning();
            for (auto it = versioning->options().constBegin(); it != versioning->options().constEnd(); ++it)
                options.emplace(it.key().toStdString(), it.value());
            for (auto it = versioning->mapStructTypes().constBegin();
                 it != versioning->mapStructTypes().constEnd(); ++it)
            {
                mapStructTypes.emplace(it.key().toStdString(),
                                       VersionContainer::FMapStructTypes{it.value().first.toStdString(),
                                                                         it.value().second.toStdString()});
            }
        }

        VersionContainer versions(game,
                                  currentDir != nullptr ? currentDir->texturePlatform()
                                                        : CUE4Parse::UE4::Assets::Exports::Texture::ETexturePlatform::DesktopMobile,
                                  {}, std::move(options), std::move(mapStructTypes));

        if (currentDir != nullptr && currentDir->versioning() != nullptr &&
            !currentDir->versioning()->customVersions().isEmpty())
        {
            auto container = std::make_shared<CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersionContainer>();
            for (const auto& cv : currentDir->versioning()->customVersions())
                container->Versions.push_back({cv.Key, cv.Version});
            versions.CustomVersions = std::move(container);
        }

        const StringComparer pathComparer = StringComparer::OrdinalIgnoreCase();

        if (gameDirectory == Constants::_FN_LIVE_TRIGGER || gameDirectory == Constants::_VAL_LIVE_TRIGGER)
        {
            // C# builds a StreamedFileProvider("FortniteLive" / "ValorantLive"), which streams the game from
            // Epic's/Riot's CDN through EpicManifestParser. None of that is ported, and silently pointing a
            // DefaultFileProvider at "fortnite-live.manifest" would look like a broken install instead.
            _unsupportedLiveService = true;
        }
        else
        {
            const QString project = projectFolderName(gameDirectory);

            if (project == QStringLiteral("StateOfDecay2"))
            {
                _provider = std::make_unique<DefaultFileProvider>(
                    toPath(gameDirectory),
                    std::vector<std::filesystem::path>{
                        toPath(localAppData() + QStringLiteral("/StateOfDecay2/Saved/Paks")),
                        toPath(localAppData() + QStringLiteral("/StateOfDecay2/Saved/DisabledPaks"))},
                    SearchOption::AllDirectories, versions, pathComparer);
            }
            else if (project == QStringLiteral("eFootball"))
            {
                _provider = std::make_unique<DefaultFileProvider>(
                    toPath(gameDirectory),
                    std::vector<std::filesystem::path>{
                        toPath(commonAppData() + QStringLiteral("/KONAMI/eFootball/ST/Download"))},
                    SearchOption::AllDirectories, versions, pathComparer);
            }
            else if (project == QStringLiteral("DeadByDaylight"))
            {
                _provider = std::make_unique<DefaultFileProvider>(
                    toPath(gameDirectory),
                    std::vector<std::filesystem::path>{
                        toPath(localAppData() +
                               QStringLiteral("/DeadByDaylight/Saved/PersistentDownloadDir/DynamicContent"))},
                    SearchOption::AllDirectories, versions, pathComparer);
            }
            else if (game == EGame::GAME_BlackStigma)
            {
                // The one game-specific arm that IS fully ported: its only difference is a case-SENSITIVE
                // path comparer.
                _provider = std::make_unique<DefaultFileProvider>(
                    toPath(gameDirectory), SearchOption::AllDirectories, versions, StringComparer::Ordinal());
            }
            else
            {
                // C# also has arms for AshEchoes (AEDefaultFileProvider), HonorofKingsWorld
                // (HoKWDefaultFileProvider) and LordOfMysteries (LoMDefaultFileProvider); all three are
                // stubs in CUE4Parse::GameTypes, so they fall back here. Their containers will register but
                // may not decrypt.
                _provider = std::make_unique<DefaultFileProvider>(
                    toPath(gameDirectory), SearchOption::AllDirectories, versions, pathComparer);
            }
        }

        // C# also sets Provider.ReadScriptData / ReadShaderMaps / ReadNaniteData here; those flags belong to
        // layers CUE4Parse has not ported yet, so the provider does not carry them.

        _gameDirectory = new GameDirectoryViewModel(this);
        _assetsFolder = new AssetsFolderViewModel(this);
        // C# also builds SearchVm, RefVm, TabControl and the IoStoreOnDemand ConfigIni here.
    }

    CUE4ParseViewModel::~CUE4ParseViewModel() = default;

    void CUE4ParseViewModel::initialize()
    {
        if (_provider == nullptr)
            return;

        _worker->begin([this](FCancellationToken& token)
        {
            token.throwIfCancellationRequested();

            // C# first configures Provider.OnDemandOptions (the chunk downloader) and, for a
            // StreamedFileProvider, downloads and registers the live manifest; then, for a local provider,
            // reads Cloud/IoStoreOnDemand.ini. All three need the HTTP layer.
            _provider->Initialize();
            _gameDirectory->addLooseFiles(_provider->LooseFileCount);
        });

        // C# ends with a Serilog line: game, platform, archive count, required-key count, loose files.
    }

    void CUE4ParseViewModel::loadVfs(
        const std::vector<std::pair<FGuid, std::shared_ptr<FAesKey>>>& aesKeys)
    {
        if (_provider == nullptr)
            return;

        _provider->SubmitKeys(aesKeys);
        _provider->PostMount();
    }

    void CUE4ParseViewModel::clearProvider()
    {
        if (_provider == nullptr)
            return;

        _assetsFolder->folders().clear();
        // C# also clears SearchVm.SearchResults and closes the search window; neither is ported.
        _provider->UnloadNonStreamedVfs();
        // C#'s GC.Collect() has no counterpart.
    }

    QString CUE4ParseViewModel::initMappings(bool /*force*/)
    {
        if (_provider == nullptr)
            return {};

        Settings::EndpointSettings* endpoint = nullptr;
        if (!Settings::UserSettings::IsEndpointValid(EEndpointType::Mapping, endpoint) || endpoint == nullptr)
        {
            _provider->MappingsContainer = nullptr;
            return {};
        }

        // C#'s first branch, and the only one that works without the network: an explicitly supplied file.
        if (endpoint->overwrite() && QFileInfo::exists(endpoint->filePath()))
        {
            // SelectMappingsProvider also handles .jmap/.jmap.gz through JmapTypeMappingsProvider, which is
            // a stub; a .jmap path would be read as a usmap and throw, so it is rejected up front.
            if (endpoint->filePath().endsWith(QStringLiteral(".jmap"), Qt::CaseInsensitive) ||
                endpoint->filePath().endsWith(QStringLiteral(".jmap.gz"), Qt::CaseInsensitive))
            {
                emit deferred(endpoint->filePath(), QStringLiteral("MappingsProvider/Jmap"));
                return {};
            }

            auto mappings = std::make_shared<CUE4Parse::MappingsProvider::Usmap::FileUsmapTypeMappingsProvider>(
                endpoint->filePath().toStdString());
            const QString fileName = QString::fromStdString(mappings->FileName());
            _provider->MappingsContainer = std::move(mappings);
            return fileName;
        }

        // C#'s second branch downloads the newest mappings from the configured endpoint and falls back to
        // the newest `*_oo.usmap` already in the cache directory. Both need the API layer.
        emit deferred(QStringLiteral("InitMappings"), QStringLiteral("ViewModels/ApiEndpoints (DynamicApi)"));
        return {};
    }

    QStringList CUE4ParseViewModel::verifyConsoleVariables() const
    {
        QStringList warnings;
        if (_provider == nullptr)
            return warnings;

        if (_provider->Versions[std::string("StripAdditiveRefPose")])
        {
            warnings.append(QStringLiteral(
                "Additive animations have their reference pose stripped, which will lead to inaccurate "
                "preview and export"));
        }

        const EGame game = _provider->Versions.Game();
        const QString projectName = QString::fromStdString(_provider->ProjectName());
        if ((game == CUE4Parse::UE4::Versions::GAME_UE4_LATEST || game == CUE4Parse::UE4::Versions::GAME_UE5_LATEST) &&
            projectName.compare(QStringLiteral("FortniteGame"), Qt::CaseInsensitive) != 0) // ignore fortnite globally
        {
            const QString displayName = QString::fromStdString(_provider->GameDisplayName());
            warnings.append(QStringLiteral("Experimental UE version selected, likely unsuitable for '%1'")
                                .arg(displayName.isEmpty() ? projectName : displayName));
        }
        return warnings;
    }

    bool CUE4ParseViewModel::loadGameLocalizedResources()
    {
        if (_localResourcesDone)
            return true;

        const auto language = Settings::UserSettings::Default()->assetLanguage();
        _localResourcesDone = _provider->TryChangeCulture(_provider->GetLanguageCode(language));
        return _localResourcesDone;
    }

    int CUE4ParseViewModel::loadLocalizedResources()
    {
        if (_provider == nullptr)
            return 0;

        const int snapshot = _localizedResourcesCount;
        loadGameLocalizedResources();
        // C# also awaits LoadHotfixedLocalizedResources, which pulls Fortnite's live hotfixes off the API.

        _localizedResourcesCount = static_cast<int>(_provider->Internationalization.Count());
        if (snapshot != _localizedResourcesCount)
        {
            // C# logs the count here and rebuilds Utils.Typefaces (the font cache, unported).
        }
        return _localizedResourcesCount;
    }

    int CUE4ParseViewModel::loadVirtualPaths()
    {
        if (_provider == nullptr || _virtualPathCount > 0)
            return _virtualPathCount;

        const auto* currentDir = Settings::UserSettings::Default()->currentDir();
        const EGame game = currentDir != nullptr ? currentDir->ueVersion() : CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        _virtualPathCount = _provider->LoadVirtualPaths(CUE4Parse::UE4::Versions::GetVersion(game));
        return _virtualPathCount;
    }
}
