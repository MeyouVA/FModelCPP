// Ported from FModel/ViewModels/ApplicationViewModel.cs
#include "ApplicationViewModel.h"

#include <QLocale>

#include "UE4/Versions/EGame.h"

#include "Encryption/Aes/FAesKey.h"
#include "UE4/VirtualFileSystem/IAesVfsReader.h"

#include "AesManagerViewModel.h"
#include "CUE4ParseViewModel.h"
#include "GameDirectoryViewModel.h"
#include "GameSelectorViewModel.h"
#include "LoadingModesViewModel.h"
#include "SettingsViewModel.h"
#include "ThreadWorkerViewModel.h"
#include "Commands/CopyCommand.h"
#include "Commands/LoadCommand.h"
#include "Commands/MenuCommand.h"
#include "Commands/RightClickMenuCommand.h"
#include "../Constants.h"
#include "../Helper.h"
#include "../Extensions/AssetCategoryExtensions.h"
#include "../Framework/FStatus.h"
#include "../Settings/DirectorySettings.h"
#include "../Settings/UserSettings.h"

namespace FModel::ViewModels
{
    using Framework::FStatus;

    namespace
    {
        // The window host's seam â€” see the header. A function-local static keeps it out of static-init order.
        std::function<bool(GameSelectorViewModel*, const QString&)>& directorySelectorHandler()
        {
            static std::function<bool(GameSelectorViewModel*, const QString&)> handler;
            return handler;
        }
    }

    void ApplicationViewModel::setDirectorySelectorHandler(
        std::function<bool(GameSelectorViewModel*, const QString&)> handler)
    {
        directorySelectorHandler() = std::move(handler);
    }

    ApplicationViewModel::ApplicationViewModel(QObject* parent)
        : ViewModel(parent)
        , _status(new FStatus(this))
        , _threadWorker(new ThreadWorkerViewModel(this))
        , _loadingModes(new LoadingModesViewModel(this))
        , _settingsView(new SettingsViewModel(this))
        , _categories(Extensions::AssetCategoryExtensions::getBaseCategories())
    {
#ifdef NDEBUG
        setBuild(EBuildKind::Release);
#else
        setBuild(EBuildKind::Debug);
#endif

        _threadWorker->setApplicationView(this);
        _loadingModes->loadCommand()->setApplicationView(this);

        // C#: `UserSettings.Default.CurrentDir = AvoidEmptyGameDirectory(false);` and then a hard
        // Environment.Exit(0) when it comes back null. The port does NOT exit â€” an unconfigured app must
        // still be constructible for the shell and the tests â€” but it does ask, when a host is installed.
        auto* settings = Settings::UserSettings::Default();
        if (settings->currentDir() == nullptr)
        {
            if (Settings::DirectorySettings* chosen = avoidEmptyGameDirectory(false))
                settings->setCurrentDir(chosen);
        }

        _cue4Parse = new CUE4ParseViewModel(_threadWorker, this);
        if (auto* provider = _cue4Parse->provider())
        {
            using CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader;
            using CUE4Parse::UE4::VirtualFileSystem::IVfsReader;

            provider->VfsRegistered = [this](IVfsReader& sender, int count)
            {
                auto* reader = dynamic_cast<IAesVfsReader*>(&sender);
                if (reader == nullptr) return;
                _status->updateStatusLabel(
                    QStringLiteral("%1 Archives (%2)").arg(count).arg(QString::fromStdString(reader->Name())),
                    QStringLiteral("Registered"));
                _cue4Parse->gameDirectory()->add(*reader);
            };
            provider->VfsMounted = [this](IVfsReader& sender, int count)
            {
                auto* reader = dynamic_cast<IAesVfsReader*>(&sender);
                if (reader == nullptr) return;
                _status->updateStatusLabel(
                    QStringLiteral("%1 Packages (%2)").arg(QLocale().toString(count),
                                                           QString::fromStdString(reader->Name())),
                    QStringLiteral("Mounted"));
                _cue4Parse->gameDirectory()->verify(*reader);
            };
            provider->VfsUnmounted = [this](IVfsReader& sender, int)
            {
                if (auto* reader = dynamic_cast<IAesVfsReader*>(&sender))
                    _cue4Parse->gameDirectory()->disable(*reader);
            };
        }

        // C# also builds CustomDirectories and AudioPlayer here.
        _aesManager = new AesManagerViewModel(_cue4Parse, this);

        _status->setStatus(EStatusKind::Ready);
    }

    Settings::DirectorySettings* ApplicationViewModel::avoidEmptyGameDirectory(bool bAlreadyLaunched)
    {
        auto* settings = Settings::UserSettings::Default();
        const QString gameDirectory = settings->gameDirectory();

        if (!bAlreadyLaunched)
        {
            if (Settings::DirectorySettings* currentDir = settings->perDirectory(gameDirectory))
                return currentDir;
        }

        const auto& handler = directorySelectorHandler();
        if (!handler)
            return nullptr; // headless: nothing to ask, and C#'s Exit(0) is deliberately not reproduced

        _status->setStatus(EStatusKind::Configuring);
        auto selector = std::make_unique<GameSelectorViewModel>(gameDirectory);
        const bool accepted = handler(selector.get(), QString());
        _status->setStatus(EStatusKind::Ready);
        if (!accepted || selector->selectedDirectory() == nullptr)
            return nullptr;

        settings->setGameDirectory(selector->selectedDirectory()->gameDirectory());
        if (!bAlreadyLaunched ||
            (settings->currentDir() != nullptr && settings->currentDir()->equals(*selector->selectedDirectory())))
        {
            return selector->selectedDirectory()->clone();
        }

        // C#'s commented-out UserSettings.Save() is left commented there too ("??? change key then change
        // game, key saved correctly what?").
        settings->setCurrentDir(selector->selectedDirectory()->clone());
        emit restartRequested();
        return nullptr;
    }

    Settings::DirectorySettings* ApplicationViewModel::addGameDirectory(const QString& directory)
    {
        const auto& handler = directorySelectorHandler();
        if (!handler)
            return nullptr;

        // C# splits on whether the selector is ALREADY open (Status == Configuring): in that case it just
        // pre-fills the open window and returns. The host handles that distinction, since it owns the window.
        auto* settings = Settings::UserSettings::Default();
        const bool alreadyOpen = _status->kind() == EStatusKind::Configuring;

        _status->setStatus(EStatusKind::Configuring);
        auto selector = std::make_unique<GameSelectorViewModel>(settings->gameDirectory());
        const bool accepted = handler(selector.get(), directory);
        _status->setStatus(EStatusKind::Ready);
        if (alreadyOpen || !accepted || selector->selectedDirectory() == nullptr)
            return nullptr;

        settings->setGameDirectory(selector->selectedDirectory()->gameDirectory());
        if (settings->currentDir() != nullptr && settings->currentDir()->equals(*selector->selectedDirectory()))
            return selector->selectedDirectory()->clone();

        settings->setCurrentDir(selector->selectedDirectory()->clone());
        emit restartRequested();
        return nullptr;
    }

    void ApplicationViewModel::updateProvider(bool isLaunch)
    {
        if (!isLaunch && !_aesManager->hasChange())
            return;

        _cue4Parse->clearProvider();
        _threadWorker->begin([this](FCancellationToken& token)
        {
            using CUE4Parse::Encryption::Aes::FAesKey;
            using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

            std::vector<std::pair<FGuid, std::shared_ptr<FAesKey>>> aes;
            if (_aesManager->aesKeys() != nullptr)
            {
                for (FileItem* item : _aesManager->aesKeys()->items())
                {
                    token.throwIfCancellationRequested(); // cancel if needed

                    // C#: `var k = x.Key.Trim(); if (k.Length != 66) k = Constants.ZERO_64_CHAR;` â€” anything
                    // that is not exactly a 0x-prefixed 256-bit key is replaced by the all-zero key rather
                    // than skipped, so the archive still gets a mount attempt.
                    QString k = item->key().trimmed();
                    if (k.size() != 66)
                        k = Constants::ZERO_64_CHAR;
                    aes.emplace_back(item->guid(), std::make_shared<FAesKey>(k.toStdString()));
                }
            }

            _cue4Parse->loadVfs(aes);
            _aesManager->setAesKeys();
        });
        raisePropertyChanged(QStringLiteral("GameDisplayName"));
    }

    Commands::RightClickMenuCommand* ApplicationViewModel::rightClickMenuCommand()
    {
        if (!_rightClickMenuCommand)
            _rightClickMenuCommand = new Commands::RightClickMenuCommand(this, this);
        return _rightClickMenuCommand;
    }

    Commands::MenuCommand* ApplicationViewModel::menuCommand()
    {
        if (!_menuCommand)
            _menuCommand = new Commands::MenuCommand(this, this);
        return _menuCommand;
    }

    Commands::CopyCommand* ApplicationViewModel::copyCommand()
    {
        if (!_copyCommand)
            _copyCommand = new Commands::CopyCommand(this, this);
        return _copyCommand;
    }

    void ApplicationViewModel::setBuild(EBuildKind value)
    {
        if (setProperty(_build, value, QStringLiteral("Build")))
            raisePropertyChanged(QStringLiteral("TitleExtra"));
    }

    void ApplicationViewModel::setIsAssetsExplorerVisible(bool value)
    {
        if (value && !Settings::UserSettings::Default()->featurePreviewNewAssetExplorer())
            return;

        setProperty(_isAssetsExplorerVisible, value, QStringLiteral("IsAssetsExplorerVisible"));
    }

    void ApplicationViewModel::setSelectedLeftTabIndex(int value)
    {
        if (value < 0 || value > 2) return;
        setProperty(_selectedLeftTabIndex, value, QStringLiteral("SelectedLeftTabIndex"));
    }

    QString ApplicationViewModel::initialWindowTitle() const
    {
        // C#: $"FModel ({APP_SHORT_COMMIT_ID} - {APP_BUILD_DATE:MMM d, yyyy})"
        const QString date = QLocale::c().toString(Constants::APP_BUILD_DATE(), QStringLiteral("MMM d, yyyy"));
        return QStringLiteral("FModel (%1 - %2)").arg(Constants::APP_SHORT_COMMIT_ID(), date);
    }

    QString ApplicationViewModel::gameDisplayName() const
    {
        // C#: CUE4Parse.Provider.GameDisplayName ?? "Unknown". The C++ provider returns an empty string
        // where C# returns null (see AbstractFileProvider.h), so emptiness is the null branch.
        if (_cue4Parse == nullptr || _cue4Parse->provider() == nullptr)
            return QStringLiteral("Unknown");

        const QString displayName = QString::fromStdString(_cue4Parse->provider()->GameDisplayName());
        return displayName.isEmpty() ? QStringLiteral("Unknown") : displayName;
    }

    QString ApplicationViewModel::titleExtra() const
    {
        // C#: $"({CurrentDir.UeVersion}){(Build != Release ? $" ({Build})" : "")}"
        const auto* currentDir = Settings::UserSettings::Default()->currentDir();
        const char* version = currentDir
            ? CUE4Parse::UE4::Versions::EGameName(currentDir->ueVersion())
            : nullptr;

        QString extra = QStringLiteral("(%1)").arg(version ? QString::fromLatin1(version) : QString());
        if (_build != EBuildKind::Release)
            extra += QStringLiteral(" (%1)").arg(buildKindName(_build));
        return extra;
    }
}
