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

    private:
        // Builders mirroring the XAML tree, top to bottom.
        void buildMenuBar();
        QWidget* buildLeftTabControl();
        QWidget* buildRightPanel();
        void buildStatusBar();

        QAction* makeCommandAction(const QString& text, const QString& parameter, const QString& shortcut = {});
        void log(const QString& message);

        // Named controls (WPF x:Name equivalents).
        QCheckBox* _featurePreviewToggle = nullptr; // "Preview New Explorer System"
        QTabWidget* _leftTabControl = nullptr;      // LeftTabControl
        QListWidget* _directoryFilesListBox = nullptr; // DirectoryFilesListBox (Archives tab)
        QTreeWidget* _assetsFolderName = nullptr;   // AssetsFolderName (Folders tab)
        QLineEdit* _assetsSearchTextBox = nullptr;  // AssetsSearchTextBox (Assets tab)
        QListWidget* _assetsListName = nullptr;     // AssetsListName (Assets tab)

        QStackedWidget* _mainStack = nullptr;       // explorer border vs TabControlName (IsAssetsExplorerVisible)
        QLineEdit* _assetsExplorerSearch = nullptr; // AssetsExplorerSearch
        QComboBox* _categoriesSelector = nullptr;   // CategoriesSelector
        QListWidget* _assetsExplorer = nullptr;     // AssetsExplorer
        QTabWidget* _tabControlName = nullptr;      // TabControlName (editor/viewer tabs)
        QTextEdit* _logRtbName = nullptr;           // LogRtbName

        Framework::FStatus* _status = nullptr;      // Status (drives the status bar)
        QLabel* _statusLabel = nullptr;             // bound to Status.Label
        QLabel* _lastRefreshLabel = nullptr;        // "Last Refresh: ..."
    };
}
