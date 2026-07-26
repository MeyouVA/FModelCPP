#pragma once
// Ported from FModel/Views/SettingsView.xaml (+ .xaml.cs) — the Settings window.
//
// The WPF window is a TreeView of six entries beside a ContentControl whose template is selected by the
// entry's Tag (OnTagDataTemplateSelector). Here that is a QTreeWidget beside a QStackedWidget, one page per
// DataTemplate, in the same order: General, Creator, Models, Keybindings, Unluac, Themes.
//
// Deliberate differences from C#:
//   * C# reaches the root view-model through ApplicationService.ApplicationView, a static service locator.
//     ApplicationService is not ported, so the window is handed the ApplicationViewModel instead — the same
//     object, passed rather than looked up.
//   * WPF's TwoWay bindings become explicit signal connections that write into UserSettings.Default the
//     moment a control changes, which is the same observable behaviour (none of the toggles set an explicit
//     UpdateSourceTrigger, so they too write immediately).
//   * Visibility of two tree entries is data-driven upstream. Unluac follows UserSettings.DecompileLua and
//     works the same here. Creator follows `CUE4Parse.Provider.ProjectName == "FortniteGame"`, and
//     CUE4ParseViewModel is not ported, so it stays hidden — which is what the WPF style's default Collapsed
//     setter yields until the trigger fires.
//   * Deferred pieces of OnClick, each blocked on its own port: RestartWithWarning (needs the directory
//     selector and a process restart), the two SettingsOut arms (CUE4ParseViewModel.LoadLocalizedResources /
//     InitMappings), and Provider.ReadScriptData / ReadShaderMaps. Everything else — Save(), the settings
//     write, closing the window — runs. The unported steps are reported through `deferred`, the seam
//     MenuCommand already uses.
//   * The browse buttons open QFileDialog rather than Ookii's Vista dialogs, and route through the
//     browseHandler seam below so a test can answer them without a modal window. C# has no such seam.
//   * The Themes page shows the JSON preview as a plain read-only QPlainTextEdit: AvalonEdit's highlighter
//     and JsonHighlightPalette (which is what the theme actually changes) are not ported, so the preview
//     renders but does not recolour. The sample document is carried over verbatim.
//   * OnDecompileLuaChanged's `await ApplicationViewModel.InitUnluac()` is dropped with the rest of the
//     native-DLL initialisers; ticking the box only reveals the Unluac page.

#include <QDialog>
#include <QList>
#include <QString>

#include <functional>

#include "../Enums.h"

class QCheckBox;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace FModel::ViewModels
{
    class ApplicationViewModel;
    class SettingsViewModel;
}

namespace FModel::Views
{
    class SettingsView : public QDialog
    {
        Q_OBJECT

    public:
        // What a browse button asks for. `Directory` is C#'s VistaFolderBrowserDialog, `MappingFile` its
        // OpenFileDialog with the USMAP filter.
        enum class BrowseKind { Directory, MappingFile };

        // Test seam with no C# counterpart: returns the chosen path, or an empty string for "cancelled".
        // Passing an empty function restores the QFileDialog default.
        static void setBrowseHandler(std::function<QString(BrowseKind, const QString& title)> handler);

        explicit SettingsView(ViewModels::ApplicationViewModel* applicationView, QWidget* parent = nullptr);

        // C#'s OnClick (the OK button).
        void onClick();

        // The browse handlers, one per output directory, named as in C#.
        void onBrowseOutput();
        void onBrowseDirectories();
        void onBrowseRawData();
        void onBrowseProperties();
        void onBrowseTexture();
        void onBrowseAudio();
        void onBrowseModels();
        void onBrowseMappings();

        // C#'s three DictionaryEditor launchers and two EndpointEditor launchers.
        void openCustomVersions();
        void openOptions();
        void openMapStructTypes();
        void openAesEndpoint();
        void openMappingEndpoint();

        // C#'s static TryParseKey, used by the CRIWARE key box. Returns whether the text parsed.
        static bool tryParseKey(const QString& text, quint64& value);

        // The JSON document the Themes page previews. C#'s JsonThemePreviewText.
        static const QString JsonThemePreviewText;

    signals:
        // Raised for a step of OnClick whose target is not ported yet (see the header note).
        void deferred(const QString& step, const QString& waitingOn);

    private:
        QWidget* buildGeneralPage();
        QWidget* buildCreatorPage();
        QWidget* buildModelsPage();
        QWidget* buildKeybindingsPage();
        QWidget* buildUnluacPage();
        QWidget* buildThemesPage();

        // C#'s constructor loop and OnSelectedItemChanged, which count only the visible entries.
        void selectLastOpenedTab();
        void onSelectedItemChanged();

        // Shared body of the five launchers: sets the status to Configuring for the modal's lifetime, exactly
        // as C# does, and reports whether OK was pressed.
        bool runConfiguringDialog(QDialog& dialog);

        static bool tryBrowse(BrowseKind kind, const QString& title, QString& path);

        ViewModels::ApplicationViewModel* _applicationView = nullptr;
        ViewModels::SettingsViewModel* _settingsView = nullptr;

        QTreeWidget* _settingsTree = nullptr;
        QStackedWidget* _pages = nullptr;
        QList<QTreeWidgetItem*> _treeItems;

        QCheckBox* _decompileLua = nullptr;
        QTreeWidgetItem* _unluacItem = nullptr;
        QTreeWidgetItem* _creatorItem = nullptr;
    };
}
