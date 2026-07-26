// Ported from FModel/ViewModels/Commands/MenuCommand.cs
#include "MenuCommand.h"

#include <QDesktopServices>
#include <QList>

#include "../../Constants.h"
#include "../../Settings/UserSettings.h"
#include "../AssetsFolderViewModel.h"

namespace FModel::ViewModels::Commands
{
    namespace
    {
        // The seam described in the header. A function-local static keeps it out of the static-init order.
        std::function<bool(const QUrl&)>& openUrlHandler()
        {
            static std::function<bool(const QUrl&)> handler;
            return handler;
        }

        std::function<bool(const QString&, ApplicationViewModel*)>& openWindowHandler()
        {
            static std::function<bool(const QString&, ApplicationViewModel*)> handler;
            return handler;
        }
    }

    void MenuCommand::setOpenWindowHandler(
        std::function<bool(const QString&, ApplicationViewModel*)> handler)
    {
        openWindowHandler() = std::move(handler);
    }

    bool MenuCommand::openWindow(const QString& window, ApplicationViewModel* contextViewModel)
    {
        if (const auto& handler = openWindowHandler())
            return handler(window, contextViewModel);
        return false;
    }

    void MenuCommand::setOpenUrlHandler(std::function<bool(const QUrl&)> handler)
    {
        openUrlHandler() = std::move(handler);
    }

    bool MenuCommand::openUrl(const QUrl& url)
    {
        if (const auto& handler = openUrlHandler())
            return handler(url);
        return QDesktopServices::openUrl(url);
    }

    void MenuCommand::setFoldersIsExpanded(AssetsFolderViewModel* root, bool expand)
    {
        if (root == nullptr)
            return;

        // C# uses a LinkedList as a work queue and keeps walking it while appending — a breadth-first
        // traversal whose node order it then reuses in reverse. A QList indexed by position behaves the
        // same way and keeps that order available for the second pass.
        QList<TreeItem*> nodes;
        for (TreeItem* folder : root->folders().items())
            nodes.append(folder);

        for (qsizetype i = 0; i < nodes.size(); ++i)
        {
            TreeItem* folder = nodes[i];

            // Collapse top-down (reduce layout updates)
            if (!expand && folder->isExpanded())
                folder->setIsExpanded(false);

            for (TreeItem* child : folder->folders().items())
                nodes.append(child);
        }

        if (!expand)
            return;

        // Expand bottom-up (reduce layout updates)
        for (qsizetype i = nodes.size() - 1; i >= 0; --i)
            nodes[i]->setIsExpanded(true);
    }

    void MenuCommand::execute(ApplicationViewModel* contextViewModel, const QVariant& parameter)
    {
        // C#'s `switch (parameter)` matches the string cases first and falls through to a TreeItem pattern.
        if (parameter.typeId() != QMetaType::QString)
        {
            // case TreeItem selectedFolder: toggles IsSelected off and back on, which is what makes the
            // explorer re-read the folder. Anything else matches no case and does nothing.
            if (auto* selectedFolder = parameter.value<TreeItem*>())
            {
                selectedFolder->setIsSelected(false);
                selectedFolder->setIsSelected(true);
            }
            return;
        }

        const QString trigger = parameter.toString();

        if (trigger == QStringLiteral("Directory_Selector"))
        {
            // C#: contextViewModel.AvoidEmptyGameDirectory(true) — reopening the selector after launch,
            // which restarts the app when a different game is chosen.
            if (contextViewModel != nullptr)
                contextViewModel->avoidEmptyGameDirectory(true);
        }
        else if (trigger == QStringLiteral("Directory_AES"))
        {
            // Helper.OpenWindow<AdonisWindow>("AES Manager", () => new AesManager().Show())
            if (!openWindow(trigger, contextViewModel))
                emit deferred(trigger, QStringLiteral("Views/AesManager (no window host installed)"));
        }
        else if (trigger == QStringLiteral("Directory_Backup"))
        {
            // Helper.OpenWindow<AdonisWindow>("Backup Manager", () => new BackupManager(...).Show())
            emit deferred(trigger, QStringLiteral("Views/BackupManager"));
        }
        else if (trigger == QStringLiteral("Directory_ArchivesInfo"))
        {
            // Hides the assets explorer, adds an "Archives Info" tab and fills it with the serialized
            // GameDirectory.DirectoryFiles.
            emit deferred(trigger, QStringLiteral("ViewModels/CUE4ParseViewModel"));
        }
        else if (trigger == QStringLiteral("Views_3dViewer"))
        {
            // contextViewModel.CUE4Parse.SnooperViewer.Run()
            emit deferred(trigger, QStringLiteral("Views/Snooper"));
        }
        else if (trigger == QStringLiteral("Views_AudioPlayer"))
        {
            emit deferred(trigger, QStringLiteral("Views/AudioPlayer"));
        }
        else if (trigger == QStringLiteral("Views_ImageMerger"))
        {
            emit deferred(trigger, QStringLiteral("Views/ImageMerger"));
        }
        else if (trigger == QStringLiteral("Settings"))
        {
            // Helper.OpenWindow<AdonisWindow>("Settings", () => new SettingsView().Show()) — the window is
            // ported; opening it needs a host (see openWindow above).
            if (!openWindow(trigger, contextViewModel))
                emit deferred(trigger, QStringLiteral("Views/SettingsView (no window host installed)"));
        }
        else if (trigger == QStringLiteral("Help_About"))
        {
            emit deferred(trigger, QStringLiteral("Views/About"));
        }
        else if (trigger == QStringLiteral("Help_Donate"))
        {
            openUrl(QUrl(Constants::DONATE_LINK));
        }
        else if (trigger == QStringLiteral("Help_Releases"))
        {
            emit deferred(trigger, QStringLiteral("Views/UpdateView"));
        }
        else if (trigger == QStringLiteral("Help_BugsReport"))
        {
            openUrl(QUrl(Constants::ISSUE_LINK));
        }
        else if (trigger == QStringLiteral("Help_Discord"))
        {
            openUrl(QUrl(Constants::DISCORD_LINK));
        }
        else if (trigger == QStringLiteral("ToolBox_Clear_Logs"))
        {
            // FLogger.ClearLogs() — FLogger lives in Views/Resources/Controls/Rtb/CustomRichTextBox.cs.
            // MainWindow keeps clearing its own log box directly until that lands.
            emit deferred(trigger, QStringLiteral("Views/Resources/Controls/Rtb (FLogger)"));
        }
        else if (trigger == QStringLiteral("ToolBox_Open_Output_Directory"))
        {
            // C# hands the raw directory to the shell; QUrl::fromLocalFile is the same thing for a path.
            openUrl(QUrl::fromLocalFile(Settings::UserSettings::Default()->outputDirectory()));
        }
        else if (trigger == QStringLiteral("ToolBox_Collapse_All"))
        {
            // await ThreadWorkerView.Begin(ct => SetFoldersIsExpanded(CUE4Parse.AssetsFolder, false, ct)).
            // The walk is ported (setFoldersIsExpanded above); the route to the tree is not — AssetsFolder
            // hangs off CUE4ParseViewModel. ToolBox_Expand_All is commented out upstream and so has no arm
            // here either.
            emit deferred(trigger, QStringLiteral("ViewModels/CUE4ParseViewModel (AssetsFolder)"));
        }
        // C#'s switch has no default: an unrecognised string simply does nothing.
    }
}
