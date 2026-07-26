#pragma once
// Ported from FModel/Views/DirectorySelector.xaml (+ .xaml.cs) — the window that picks which game to load.
//
// The layout mirrors the XAML: the detected-directory list on the left, the UE-version combo and the
// "manually add a game" expander below it, and OK at the bottom right.
//
// Deliberate differences from C#:
//   * A QDialog, so `DialogResult = true; Close();` is accept() and the caller reads the exec() result.
//     ApplicationViewModel opens it through its directorySelectorHandler seam, which is what keeps the
//     view-model layer free of any dependency on Views.
//   * Ookii's VistaFolderBrowserDialog becomes QFileDialog::getExistingDirectory, behind a `browseHandler`
//     seam (as in SettingsView) so a test can answer the picker without showing one.
//   * Bindings are explicit: the list writes SelectedDirectory, the combo writes its UeVersion, and both
//     re-read on selection change.

#include <functional>

#include <QDialog>
#include <QString>

class QComboBox;
class QGroupBox;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace FModel::ViewModels { class GameSelectorViewModel; }

namespace FModel::Views
{
    class DirectorySelector : public QDialog
    {
        Q_OBJECT

    public:
        explicit DirectorySelector(ViewModels::GameSelectorViewModel* gameSelectorViewModel,
                                   QWidget* parent = nullptr);

        // C#'s AddManualGame(string directory): expands the manual panel and pre-fills both fields.
        void addManualGame(const QString& directory);

        // Test seam with no C# counterpart: answers the folder picker. Return an empty string for "cancelled".
        static void setBrowseHandler(std::function<QString(const QString& caption)> handler);

    private slots:
        void onBrowseDirectories();
        void onBrowseManualDirectories();
        void onAddDirectory();
        void onDeleteDirectory();
        void onClick(); // OK

    private:
        void refreshList();
        void syncFromSelection();

        ViewModels::GameSelectorViewModel* _gameSelector = nullptr;

        QListWidget* _detectedDirectories = nullptr;
        QComboBox* _ueVersions = nullptr;
        QGroupBox* _manualGameExpander = nullptr;   // ManualGameExpander
        QLineEdit* _helloMyNameIsGame = nullptr;    // HelloMyNameIsGame
        QLineEdit* _helloGameMyNameIsDirectory = nullptr; // HelloGameMyNameIsDirectory
        QPushButton* _deleteButton = nullptr;
    };
}
