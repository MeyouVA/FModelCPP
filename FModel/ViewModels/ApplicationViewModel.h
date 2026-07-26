#pragma once
// Ported from FModel/ViewModels/ApplicationViewModel.cs — the root view-model the whole window binds to.
//
// The game-loading half is now here: CUE4Parse (the provider host), AesManager, the ThreadWorker every step
// runs through, the three Vfs event handlers that keep the Archives tab in step with the provider, and
// UpdateProvider.
//
// Deferred (each blocked on its own port):
//   * CustomDirectories, AudioPlayer.
//   * RestartWithWarning / Restart — a process restart plus a modal warning.
//     (AvoidEmptyGameDirectory / AddGameDirectory are ported, over the DirectorySelector window.)
//   * The static InitVgmStream / InitImGuiSettings / InitOodle / InitZlib / InitDetex / InitUnluac helpers,
//     which download and initialise native DLLs. Oodle and Zstd DO have ported initialisers in CUE4Parse
//     (OodleHelper/ZstdHelper), and MainWindow calls them; what is missing is only the download half.
//
// Deliberate differences from C#:
//   * C#'s constructor calls AvoidEmptyGameDirectory and hard-exits when no game is chosen. With the
//     directory selector unported there is nothing to ask, so the constructor leaves CurrentDir alone and
//     does not exit — an unconfigured app must still be constructible for the shell and the tests.
//   * Build is a `private init` property in C# fixed by the DEBUG/RELEASE compilation symbols; here it is
//     resolved from NDEBUG once in the constructor and exposed read-only.
//   * Status is owned (a QObject child) rather than GC-managed.
//   * titleExtra() tolerates a null CurrentDir and renders an empty version. C# cannot reach that state
//     because its constructor exits the process first; this port can, per the point above.

#include <functional>

#include <QList>
#include <QString>

#include "../Enums.h"
#include "../Framework/ViewModel.h"

namespace FModel::Framework { class FStatus; }
namespace FModel::Settings { class DirectorySettings; }

namespace FModel::ViewModels
{
    class AesManagerViewModel;
    class CUE4ParseViewModel;
    class GameSelectorViewModel;
    class LoadingModesViewModel;
    class SettingsViewModel;
    class ThreadWorkerViewModel;

    namespace Commands
    {
        class CopyCommand;
        class MenuCommand;
        class RightClickMenuCommand;
    }

    class ApplicationViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit ApplicationViewModel(QObject* parent = nullptr);

        EBuildKind build() const { return _build; }
        Framework::FStatus* status() const { return _status; }
        LoadingModesViewModel* loadingModes() const { return _loadingModes; }
        // C#'s `public SettingsViewModel SettingsView { get; }`, built by the constructor. Owned here (a
        // QObject child) rather than left to the GC.
        SettingsViewModel* settingsView() const { return _settingsView; }

        CUE4ParseViewModel* cue4Parse() const { return _cue4Parse; }
        AesManagerViewModel* aesManager() const { return _aesManager; }
        // C#'s ApplicationService.ThreadWorkerView — a process-wide singleton there, owned here.
        ThreadWorkerViewModel* threadWorker() const { return _threadWorker; }

        // C#'s UpdateProvider(bool isLaunch): re-submits every AES key in the manager and remounts. A
        // non-launch call with no key change does nothing, which is what makes closing the AES manager with
        // no edits free.
        void updateProvider(bool isLaunch);

        // C#'s AvoidEmptyGameDirectory(bool bAlreadyLaunched). Opens the DirectorySelector through the
        // handler seam below, since this layer cannot depend on Views. Returns the chosen directory, or null
        // when the user cancelled (or when a restart is required, which C# triggers and this reports).
        Settings::DirectorySettings* avoidEmptyGameDirectory(bool bAlreadyLaunched);
        // C#'s AddGameDirectory(string): the same flow, pre-filled with a manually browsed directory.
        Settings::DirectorySettings* addGameDirectory(const QString& directory);

        // The window host installs this (MainWindow does), the same way MenuCommand's openWindowHandler
        // works: it shows the DirectorySelector for `selector` and returns whether OK was pressed.
        // `manualDirectory`, when non-empty, is C#'s AddManualGame pre-fill.
        static void setDirectorySelectorHandler(
            std::function<bool(GameSelectorViewModel* selector, const QString& manualDirectory)> handler);

        // C#'s `public XCommand XCommand => _x ??= new XCommand(this);` — lazily built, owned by this
        // view-model (C# leaves them to the GC), and handed the view-model itself as their context.
        Commands::RightClickMenuCommand* rightClickMenuCommand();
        Commands::MenuCommand* menuCommand();
        Commands::CopyCommand* copyCommand();

        // C#'s `IEnumerable<EAssetCategory> Categories` — the base categories only.
        const QList<EAssetCategory>& categories() const { return _categories; }

        bool isAssetsExplorerVisible() const { return _isAssetsExplorerVisible; }
        // Refuses to turn on while the FeaturePreviewNewAssetExplorer setting is off (C# returns early).
        void setIsAssetsExplorerVisible(bool value);

        int selectedLeftTabIndex() const { return _selectedLeftTabIndex; }
        // Ignores anything outside [0, 2] — the window has exactly three left tabs.
        void setSelectedLeftTabIndex(int value);

        QString initialWindowTitle() const;
        QString gameDisplayName() const;
        QString titleExtra() const;

    signals:
        // Where C# pops the "a restart is needed" message box and relaunches the process.
        void restartRequested();

    private:
        void setBuild(EBuildKind value);

        EBuildKind _build = EBuildKind::Unknown;
        Framework::FStatus* _status = nullptr;
        ThreadWorkerViewModel* _threadWorker = nullptr;
        LoadingModesViewModel* _loadingModes = nullptr;
        SettingsViewModel* _settingsView = nullptr;
        CUE4ParseViewModel* _cue4Parse = nullptr;
        AesManagerViewModel* _aesManager = nullptr;
        QList<EAssetCategory> _categories;
        Commands::RightClickMenuCommand* _rightClickMenuCommand = nullptr;
        Commands::MenuCommand* _menuCommand = nullptr;
        Commands::CopyCommand* _copyCommand = nullptr;
        bool _isAssetsExplorerVisible = false;
        int _selectedLeftTabIndex = 0;
    };
}
