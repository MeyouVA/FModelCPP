// Ported from FModel/MainWindow.xaml (+ MainWindow.xaml.cs) — the shell/layout only.
// This is the first slice of the FModel app port: it mirrors the WPF window's structure (menu bar, the left
// 3-tab control, the right explorer/editor + log area, and the status bar) as a Qt Widgets window. The deep
// wiring (ApplicationViewModel / CUE4Parse provider / thread worker) is NOT ported yet, so menu actions and
// controls are inert placeholders that log to the output pane. Named widgets keep their WPF x:Name (as member
// names + objectName) so the later view-model layer can find them.
#pragma once

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QTabWidget;
class QTextEdit;
class QTreeWidget;
class QLabel;

namespace FModel::Framework { class FStatus; }
namespace FModel::ViewModels { class ApplicationViewModel; }

namespace FModel
{
    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);

    private slots:
        // Stand-in for the WPF MenuCommand: every menu item routes here with its CommandParameter (in the action's
        // data) until the real command layer is ported.
        void onMenuCommand(const QString& parameter);
        void onClearLogs();
        // The Load button — C#'s LoadingModes.LoadCommand.Execute(DirectoryFilesListBox.SelectedItems).
        void onLoad();
        void onArchiveSelected(int row);

    private:
        // C#'s MainWindow.OnLoaded: refresh AES, init the native libraries, scan the game directory, read
        // the keys, mount, then the mappings. Runs once, after the window is shown.
        void runStartupSequence();
        // Rebuilds the Archives tab list and the Folders tree from the view-models.
        void refreshArchivesList();
        void refreshFoldersTree();

        // Builders mirroring the XAML tree, top to bottom.
        void buildMenuBar();
        QWidget* buildLeftTabControl();
        QWidget* buildRightPanel();
        void buildStatusBar();

        QAction* makeCommandAction(const QString& text, const QString& parameter, const QString& shortcut = {});
        // The window host MenuCommand's window arms call through (see MenuCommand::setOpenWindowHandler).
        void openSettings(ViewModels::ApplicationViewModel* contextViewModel);
        void openAesManager(ViewModels::ApplicationViewModel* contextViewModel);
        void log(const QString& message);

        // Named controls (WPF x:Name equivalents).
        QCheckBox* _featurePreviewToggle = nullptr; // "Preview New Explorer System"
        QTabWidget* _leftTabControl = nullptr;      // LeftTabControl
        QComboBox* _loadingMode = nullptr;          // LoadingMode
        QListWidget* _directoryFilesListBox = nullptr; // DirectoryFilesListBox (Archives tab)
        QLabel* _archiveMountPoint = nullptr;       // the Archives tab INFORMATION block
        QLabel* _archiveFileCount = nullptr;
        QLabel* _archiveIsEncrypted = nullptr;
        QLabel* _archiveGuid = nullptr;
        QTreeWidget* _assetsFolderName = nullptr;   // AssetsFolderName (Folders tab)
        QLineEdit* _assetsSearchTextBox = nullptr;  // AssetsSearchTextBox (Assets tab)
        QListWidget* _assetsListName = nullptr;     // AssetsListName (Assets tab)

        QStackedWidget* _mainStack = nullptr;       // explorer border vs TabControlName (IsAssetsExplorerVisible)
        QLineEdit* _assetsExplorerSearch = nullptr; // AssetsExplorerSearch
        QComboBox* _categoriesSelector = nullptr;   // CategoriesSelector
        QListWidget* _assetsExplorer = nullptr;     // AssetsExplorer
        QTabWidget* _tabControlName = nullptr;      // TabControlName (editor/viewer tabs)
        QTextEdit* _logRtbName = nullptr;           // LogRtbName

        // The WPF window's DataContext. Owned by the window; the shell reads Status/Categories off it and
        // routes the two guarded view-state properties through it.
        ViewModels::ApplicationViewModel* _applicationView = nullptr;

        Framework::FStatus* _status = nullptr;      // Status (= _applicationView->status(), not owned)
        QLabel* _statusLabel = nullptr;             // bound to Status.Label
        QLabel* _lastRefreshLabel = nullptr;        // "Last Refresh: ..."
    };
}
