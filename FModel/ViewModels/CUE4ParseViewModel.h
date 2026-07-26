#pragma once
// Ported from FModel/ViewModels/CUE4ParseViewModel.cs — the provider host: it owns the CUE4Parse file
// provider, the archives list, the folder tree, and every step of bringing a game online.
//
// THE C# FILE IS ~1,800 LINES AND THIS PORT COVERS THE LOADING HALF ONLY. What is here:
//   * the constructor's provider selection (the per-game extra directories and path comparers),
//   * Initialize   — scan the game directory for containers and loose files,
//   * LoadVfs      — submit AES keys, mount what they open, then PostMount,
//   * ClearProvider, VerifyConsoleVariables,
//   * InitMappings — the LOCAL branch (a .usmap the user points at), which is what makes a UE5 game
//                    readable at all; the download branch needs the HTTP layer,
//   * LoadLocalizedResources and LoadVirtualPaths.
// What is not: everything from ExtractSelected downwards — extraction, export, the JSON/metadata/references
// tabs, Lua decompilation, audio playback, the Snooper viewport — plus SearchVm/RefVm/TabControl, the
// Wwise/FMod/CriWare lazy providers, and the API-backed steps (RefreshAes, InitInformation, the mappings
// download, InitInformation's news). Each is called out at its site.
//
// Deliberate differences from C#:
//   * The two StreamedFileProvider games (Fortnite Live, Valorant Live) need EpicManifestParser and the
//     chunk downloader; the constructor's two trigger arms are recognised and rejected with a clear error
//     rather than silently building a local provider for a directory that does not exist.
//   * The four game-specific provider subclasses (AshEchoes, HonorofKingsWorld, LordOfMysteries and the
//     StreamedFileProvider family) are stubs in CUE4Parse, so those arms fall back to DefaultFileProvider.
//     BlackStigma's arm is fully ported, because its only difference is an ordinal path comparer.
//   * `Provider` is a std::unique_ptr the view-model owns; C# holds a reference and lets the GC decide.
//   * Everything runs on the caller's thread (see ThreadWorkerViewModel.h), so the Task/Parallel wrappers
//     collapse. The steps keep their names and order.

#include <memory>
#include <utility>
#include <vector>

#include <QObject>
#include <QString>

#include "Encryption/Aes/FAesKey.h"
#include "FileProvider/Vfs/AbstractVfsFileProvider.h"
#include "UE4/Objects/Core/Misc/FGuid.h"

#include "AssetsFolderViewModel.h"
#include "GameDirectoryViewModel.h"
#include "ThreadWorkerViewModel.h"
#include "../Framework/ViewModel.h"

namespace FModel::ViewModels
{
    class CUE4ParseViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        // C#'s parameterless constructor reads UserSettings.Default.CurrentDir; so does this. `worker` is
        // C#'s ApplicationService.ThreadWorkerView (unported locator), handed in instead.
        explicit CUE4ParseViewModel(ThreadWorkerViewModel* worker, QObject* parent = nullptr);
        ~CUE4ParseViewModel() override;

        CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider* provider() const { return _provider.get(); }
        GameDirectoryViewModel* gameDirectory() const { return _gameDirectory; }
        AssetsFolderViewModel* assetsFolder() const { return _assetsFolder; }

        int exportedCount() const { return _exportedCount; }
        int failedExportCount() const { return _failedExportCount; }

        // True when the requested game directory is one of the two live-service triggers, which need the
        // unported streamed provider. The constructor leaves `provider()` null in that case.
        bool isUnsupportedLiveService() const { return _unsupportedLiveService; }

        // C#'s Initialize(): scans the directory. Registers every container it finds (raising VfsRegistered
        // per archive) and collects loose files.
        void initialize();

        // C#'s LoadVfs: hand the keys over, mount whatever they unlock, then PostMount (which is what reads
        // the ini configs and fills ProjectName).
        void loadVfs(const std::vector<std::pair<CUE4Parse::UE4::Objects::Core::Misc::FGuid,
                                                 std::shared_ptr<CUE4Parse::Encryption::Aes::FAesKey>>>& aesKeys);

        void clearProvider();

        // C#'s InitMappings. Only the local-file branch is ported: when the Mapping endpoint is marked
        // Overwrite and its FilePath exists, that .usmap becomes the provider's mappings container. Returns
        // the file name that was loaded, or an empty string.
        QString initMappings(bool force = false);

        // C#'s VerifyConsoleVariables: two warnings, returned rather than logged (FLogger is unported).
        QStringList verifyConsoleVariables() const;

        // C#'s LoadLocalizedResources — the hotfix half needs the API and is skipped. Returns the resource
        // count, and only re-reads when the count would change, as upstream does.
        int loadLocalizedResources();
        int localizedResourcesCount() const { return _localizedResourcesCount; }

        // C#'s LoadVirtualPaths: runs once (a second call is a no-op while the count is non-zero).
        int loadVirtualPaths();

    signals:
        // Raised where C# would have called an unported subsystem, so the window can report it.
        void deferred(const QString& what, const QString& waitingOn);

    private:
        bool loadGameLocalizedResources();

        ThreadWorkerViewModel* _worker = nullptr;
        std::unique_ptr<CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider> _provider;
        GameDirectoryViewModel* _gameDirectory = nullptr;
        AssetsFolderViewModel* _assetsFolder = nullptr;

        bool _unsupportedLiveService = false;
        int _exportedCount = 0;
        int _failedExportCount = 0;
        int _localizedResourcesCount = 0;
        bool _localResourcesDone = false;
        int _virtualPathCount = 0;
    };
}
