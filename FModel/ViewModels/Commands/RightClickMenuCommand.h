#pragma once
// Ported from FModel/ViewModels/Commands/RightClickMenuCommand.cs — the explorer's context menu: the four
// "show" actions and the seven "save" actions.
//
// C#'s Execute is one method: resolve the trigger into (action, show type, bulk type), then run the work on
// the ThreadWorker. The *dispatch* is what this slice ports — every table and every piece of path/grouping
// arithmetic — as static functions that can be exercised without a loaded game. The work itself calls
// CUE4ParseViewModel (Extract / ShowMetadata / Decompile / FindReferences / ExportData / ExportFolder /
// ExtractFolder), ThreadWorkerViewModel and FLogger, none of which are ported; execute() therefore resolves
// the trigger — including its throw on an unknown one — and then raises deferred().
//
// Deliberate differences from C#:
//   * EAction and EShowAssetType are private nested enums in C#. They are public here so the resolution
//     table is testable; nothing else about them changes.
//   * C# throws ArgumentOutOfRangeException("Unsupported asset action.") for an unknown trigger; the port
//     throws std::out_of_range with the same message. Note this escapes on the UI thread in C# too — the
//     switch runs before ThreadWorkerView.Begin.
//   * `parameter` is the same two-element pack CopyCommand takes: {trigger, selection}. All three of C#'s
//     selection element types are accepted now: GameFile* (the search view passes one directly),
//     GameFileViewModel* (unwrapped to .Asset) and TreeItem* folders.
//   * C#'s GroupBy is lazy and keyed by string; groupAssets() materialises the same thing, keeping both
//     first-seen key order and within-group order, because that order decides which asset an "update" run
//     writes last.

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include "../../Enums.h"
#include "../../Framework/ViewModelCommand.h"
// ViewModelCommand holds a QPointer<TContextViewModel>, which needs the complete type — a forward
// declaration is not enough for the base-class instantiation. ApplicationViewModel.h only forward-declares
// the commands in return, so there is no cycle.
#include "../ApplicationViewModel.h"

namespace CUE4Parse::FileProvider::Objects { class GameFile; }

namespace FModel::ViewModels
{
    class TreeItem;

    namespace Commands
    {
        class RightClickMenuCommand : public Framework::ViewModelCommand<ApplicationViewModel>
        {
            Q_OBJECT

        public:
            explicit RightClickMenuCommand(ApplicationViewModel* contextViewModel, QObject* parent = nullptr)
                : ViewModelCommand(contextViewModel, parent) {}

            enum class EAction
            {
                Show,
                Export,
            };

            enum class EShowAssetType
            {
                None,
                JSON,
                Metadata,
                References,
                Decompile,
            };

            // C#'s `var (action, showtype, bulktype) = trigger switch { ... }`.
            struct FTriggerAction
            {
                EAction Action = EAction::Show;
                EShowAssetType ShowType = EShowAssetType::None;
                EBulkType BulkType = EBulkType::None;
            };
            // Throws std::out_of_range("Unsupported asset action.") — see the header note.
            static FTriggerAction resolveTrigger(const QString& trigger);

            // C#'s `var (dirType, filetype) = bulktype switch { ... }`, reading UserSettings.Default. An
            // unhandled bulk type yields (null, null) there and empty strings here; the caller returns on it.
            struct FBulkTarget
            {
                QString Directory;
                QString FileType;
            };
            static FBulkTarget resolveBulkTarget(EBulkType bulkType);

            // C#'s `assets.GroupBy(gf => gf.Directory)` plus the per-group `update` flag and the bulk mask it
            // implies (`bulktype | (update ? EBulkType.Auto : EBulkType.None)`).
            struct FAssetGroup
            {
                QString Directory;
                QList<CUE4Parse::FileProvider::Objects::GameFile*> Assets;
                bool Update = false;
                EBulkType Bulk = EBulkType::None;
            };
            static QList<FAssetGroup> groupAssets(const QList<CUE4Parse::FileProvider::Objects::GameFile*>& assets,
                                                  EBulkType bulkType);

            // C#'s Path.Combine(dirType, KeepDirectoryStructure ? directory : directory.SubstringAfterLast('/'))
            // .Replace('\\', '/') — the path LogExport links to. The folder loop feeds it
            // `folder.PathAtThisPoint`; the asset loop feeds it the group's directory.
            static QString exportPath(const QString& dirType, const QString& directory);

            // C#'s split of the untyped selection into folders and assets, in one pass. A GameFileViewModel
            // contributes its .Asset; a TreeItem is a folder; anything else is dropped (C#'s `_ => null`).
            struct FSelection
            {
                QList<TreeItem*> Folders;
                QList<CUE4Parse::FileProvider::Objects::GameFile*> Assets;
            };
            static FSelection splitSelection(const QVariantList& selection);

            // Declaring the two-argument overload would otherwise hide Command's one-argument entry point,
            // which is what the UI actually calls.
            using ViewModelCommand::execute;
            void execute(ApplicationViewModel* contextViewModel, const QVariant& parameter) override;

        signals:
            // Raised once the trigger has resolved, because the work behind it is not ported (see above):
            // the trigger, and how many assets it would have run over. Folders are not counted — C# runs
            // them through a different action.
            void deferred(const QString& parameter, int assetCount);
        };
    }
}
