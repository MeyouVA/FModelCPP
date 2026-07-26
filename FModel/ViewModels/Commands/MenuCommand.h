#pragma once
// Ported from FModel/ViewModels/Commands/MenuCommand.cs — the command behind every menu-bar entry and the
// two log-area buttons.
//
// C#'s Execute is one switch over the parameter string. Every arm is present here in the same order, but
// most of them drive a window or view-model that is not ported yet (AesManager, BackupManager, SettingsView,
// About, UpdateView, AudioPlayer, ImageMerger, the Snooper viewport, CUE4ParseViewModel, FLogger,
// ThreadWorkerViewModel, TreeItem). Those arms call deferred() instead of doing nothing silently, so the
// window can report exactly which piece is missing — this is what replaced MainWindow's onMenuCommand
// stand-in. Each carries the name of the port it waits on.
//
// Deliberate differences from C#:
//   * `Process.Start(new ProcessStartInfo { UseShellExecute = true })` becomes QDesktopServices::openUrl,
//     which is the same "hand it to the shell" behaviour for both a URL and a directory path.
//   * That call goes through the openUrlHandler seam below. C# launches the browser directly, which a test
//     cannot observe; the handler lets one record the launch instead. It has no C# counterpart.
//   * C#'s window arms read `Helper.OpenWindow<AdonisWindow>("Title", () => new XView().Show())`, which
//     re-focuses an already-open window of that title instead of opening a second one. Helper is not ported,
//     and this layer cannot construct a QWidget without depending on Views, so the arms go through the
//     openWindowHandler seam below — the window host (MainWindow) installs it. With no handler installed the
//     arm falls back to deferred(), which is also the honest answer for a headless process.
//   * C#'s Execute is `async void` because of the ToolBox_Collapse_All arm alone; with ThreadWorkerViewModel
//     unported there is nothing to await, so execute() is a plain override. setFoldersIsExpanded — the walk
//     that arm runs — IS ported now that TreeItem exists, and is public so it can be driven directly; what
//     the arm still lacks is the route to the tree, which hangs off the unported CUE4ParseViewModel. So the
//     arm stays deferred while the walk it would run is real and tested. `Thread.Yield()` and the
//     CancellationToken checks drop out with the threading layer.
//   * The `case TreeItem selectedFolder:` arm is ported: the parameter carries a TreeItem*, and re-selecting
//     it (false then true) is what forces the explorer to re-read the folder. Any OTHER non-string parameter
//     matches no case in C# and now does nothing here either.

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <functional>

#include "../../Framework/ViewModelCommand.h"
// ViewModelCommand holds a QPointer<TContextViewModel>, which needs the complete type — a forward
// declaration is not enough for the base-class instantiation. ApplicationViewModel.h only forward-declares
// the commands in return, so there is no cycle.
#include "../ApplicationViewModel.h"

namespace FModel::ViewModels
{
    class AssetsFolderViewModel;
    class TreeItem;

    namespace Commands
    {
        class MenuCommand : public Framework::ViewModelCommand<ApplicationViewModel>
        {
            Q_OBJECT

        public:
            explicit MenuCommand(ApplicationViewModel* contextViewModel, QObject* parent = nullptr)
                : ViewModelCommand(contextViewModel, parent) {}

            // Declaring the two-argument overload would otherwise hide Command's one-argument entry point,
            // which is what the UI actually calls.
            using ViewModelCommand::execute;
            void execute(ApplicationViewModel* contextViewModel, const QVariant& parameter) override;

            // Test seam with no C# counterpart (see the header note). Passing an empty function restores
            // QDesktopServices::openUrl.
            static void setOpenUrlHandler(std::function<bool(const QUrl&)> handler);
            static bool openUrl(const QUrl& url);

            // The window host's seam (see the header note). Returns whether the window was opened; false
            // routes the arm to deferred().
            static void setOpenWindowHandler(std::function<bool(const QString& window,
                                                               ApplicationViewModel* contextViewModel)> handler);
            static bool openWindow(const QString& window, ApplicationViewModel* contextViewModel);

            // C#'s private SetFoldersIsExpanded(AssetsFolderViewModel root, bool expand, CancellationToken).
            // Public here so it is reachable without the unported view-model that would call it.
            static void setFoldersIsExpanded(AssetsFolderViewModel* root, bool expand);

        signals:
            // Raised by an arm whose target is not ported yet: the menu parameter, and the name of the port
            // it waits on.
            void deferred(const QString& parameter, const QString& waitingOn);
        };
    }
}
