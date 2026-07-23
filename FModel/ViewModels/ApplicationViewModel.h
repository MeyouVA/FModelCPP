#pragma once
// Ported from FModel/ViewModels/ApplicationViewModel.cs — the root view-model the whole window binds to.
//
// This slice ports the state the shell can already show: the build kind, the FStatus the status bar reads,
// the asset categories, the two guarded view-state properties, the window-title strings, and the loading-mode
// list. Everything that needs a loaded game is deferred, because each of those is a large view-model of its
// own; the members are listed in the TODOs below in the order the C# constructor builds them.
//
// Deferred (each blocked on its own port):
//   * CUE4Parse (CUE4ParseViewModel, 1590 lines) and with it the VfsRegistered/VfsMounted/VfsUnmounted
//     handlers, ClearProvider/LoadVfs and UpdateProvider. gameDisplayName() returns "Unknown" until then,
//     which is also what C# yields when the provider has no display name.
//   * CustomDirectories, SettingsView, AesManager, AudioPlayer.
//   * RightClickMenuCommand / MenuCommand / CopyCommand (FModel/ViewModels/Commands).
//   * AvoidEmptyGameDirectory / AddGameDirectory / RestartWithWarning / Restart — all four drive the
//     DirectorySelector window and a process restart, so they need the Views layer.
//   * The static InitVgmStream / InitImGuiSettings / InitOodle / InitZlib / InitDetex / InitUnluac helpers,
//     which download and initialise native DLLs.
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

#include <QList>
#include <QString>

#include "../Enums.h"
#include "../Framework/ViewModel.h"

namespace FModel::Framework { class FStatus; }

namespace FModel::ViewModels
{
    class LoadingModesViewModel;

    class ApplicationViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit ApplicationViewModel(QObject* parent = nullptr);

        EBuildKind build() const { return _build; }
        Framework::FStatus* status() const { return _status; }
        LoadingModesViewModel* loadingModes() const { return _loadingModes; }

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

    private:
        void setBuild(EBuildKind value);

        EBuildKind _build = EBuildKind::Unknown;
        Framework::FStatus* _status = nullptr;
        LoadingModesViewModel* _loadingModes = nullptr;
        QList<EAssetCategory> _categories;
        bool _isAssetsExplorerVisible = false;
        int _selectedLeftTabIndex = 0;
    };
}
