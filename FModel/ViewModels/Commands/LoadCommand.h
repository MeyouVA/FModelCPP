#pragma once
// Ported from FModel/ViewModels/Commands/LoadCommand.cs — the Load button.
//
// Upstream's own summary: "this will always load all files no matter the loading mode; however what this
// does is filtering what to show to the user". The provider is already mounted by the time this runs; the
// command decides which of its files become tree nodes.
//
// Deliberate differences from C#:
//   * Three of the five loading modes are ported: Multiple (the archives ticked in the Archives tab), All,
//     and AllButPatched (one entry per path, keeping the highest-priority archive). AllButNew and
//     AllButModified read a .fbkp backup file, which needs the backup reader and its LZ4 frame decoder;
//     they raise deferred() instead.
//   * `Multiple` with an empty selection falls through to `All` — including C#'s side effect of writing
//     ELoadingMode::All back into settings, which is why an empty-selection load is sticky.
//   * The Discord presence update, the search view-model reset and the debug timing log are dropped with
//     their unported subsystems.
//   * The status label formatting `{count:### ### ###}` (spaces every three digits) is reproduced.

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include "FileProvider/Vfs/AbstractVfsFileProvider.h"

#include "../../Enums.h"
#include "../../Framework/ViewModelCommand.h"
#include "../LoadingModesViewModel.h"

namespace CUE4Parse::FileProvider::Objects { class GameFile; }

namespace FModel::ViewModels
{
    class ApplicationViewModel;
    class FileItem;

    namespace Commands
    {
        class LoadCommand : public Framework::ViewModelCommand<LoadingModesViewModel>
        {
            Q_OBJECT

        public:
            explicit LoadCommand(LoadingModesViewModel* contextViewModel, QObject* parent = nullptr)
                : ViewModelCommand(contextViewModel, parent) {}

            // C# reads ApplicationService.ApplicationView; the locator is unported, so the app view-model is
            // handed over by ApplicationViewModel when it builds the command.
            void setApplicationView(ApplicationViewModel* applicationView) { _applicationView = applicationView; }

            using ViewModelCommand::execute;
            void execute(LoadingModesViewModel* contextViewModel, const QVariant& parameter) override;

            // C#'s FilterDirectoryFilesToDisplay: a null selection means "everything". Static so the filter
            // can be exercised without a window.
            static QList<CUE4Parse::FileProvider::Objects::GameFile*> filterDirectoryFiles(
                CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider& provider,
                const QList<FileItem*>* directoryFiles);

            // C#'s FilterPacthedFilesToDisplay (sic): one entry per path, the highest ReadOrder winning.
            static QList<CUE4Parse::FileProvider::Objects::GameFile*> filterPatchedFiles(
                CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider& provider);

            // C#'s `$"{count:### ### ###} Packages"`.
            static QString packageCountLabel(int count);

        signals:
            // Raised for a load the user asked for that cannot run: no keys for an encrypted archive, or no
            // files at all. Both are FLogger errors upstream.
            void refused(const QString& reason);
            void deferred(const QString& what, const QString& waitingOn);

        private:
            ApplicationViewModel* _applicationView = nullptr;
        };
    }
}
