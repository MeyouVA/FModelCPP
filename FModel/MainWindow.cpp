// Ported from FModel/MainWindow.xaml (+ MainWindow.xaml.cs) — shell/layout only. See MainWindow.h.
#include "MainWindow.h"

#include "Compression/OodleHelper.h"
#include "Compression/ZstdHelper.h"
#include "FileProvider/Vfs/AbstractVfsFileProvider.h"

#include "Enums.h"
#include "Framework/FStatus.h"
#include "Settings/UserSettings.h"
#include "ViewModels/ApplicationViewModel.h"
#include "ViewModels/AssetsFolderViewModel.h"
#include "ViewModels/CUE4ParseViewModel.h"
#include "ViewModels/GameDirectoryViewModel.h"
#include "ViewModels/GameSelectorViewModel.h"
#include "ViewModels/LoadingModesViewModel.h"
#include "ViewModels/ThreadWorkerViewModel.h"
#include "ViewModels/AesManagerViewModel.h"
#include "ViewModels/Commands/LoadCommand.h"
#include "ViewModels/Commands/MenuCommand.h"
#include "Views/AesManager.h"
#include "Views/DirectorySelector.h"
#include "Views/SettingsView.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QColor>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <functional>

namespace FModel
{
    namespace
    {
        // A centred caption flanked by rules — the WPF CustomSeparator with a Tag ("GAME ARCHIVES", "INFORMATION").
        QWidget* captionSeparator(const QString& caption)
        {
            auto* box = new QWidget;
            auto* row = new QHBoxLayout(box);
            row->setContentsMargins(0, 6, 0, 2);
            auto makeLine = [] { auto* l = new QFrame; l->setFrameShape(QFrame::HLine); l->setFrameShadow(QFrame::Sunken); return l; };
            auto* label = new QLabel(caption);
            label->setStyleSheet(QStringLiteral("color:#9a9aa5;font-size:10px;"));
            row->addWidget(makeLine(), 1);
            row->addWidget(label);
            row->addWidget(makeLine(), 1);
            return box;
        }

        // The INFORMATION grid: value on the left, caption right-aligned (matching FModel's two-column info blocks).
        QGridLayout* newInfoGrid()
        {
            auto* grid = new QGridLayout;
            grid->setColumnStretch(1, 1);
            grid->setVerticalSpacing(2);
            grid->setContentsMargins(0, 0, 0, 5);
            return grid;
        }

        void addInfoRow(QGridLayout* grid, const QString& value, const QString& caption)
        {
            const int r = grid->rowCount();
            auto* v = new QLabel(value);
            v->setTextInteractionFlags(Qt::TextSelectableByMouse);
            grid->addWidget(v, r, 0, Qt::AlignLeft | Qt::AlignVCenter);
            grid->addWidget(new QLabel(caption), r, 1, Qt::AlignRight | Qt::AlignVCenter);
        }

        // Same row, but the caller keeps the value label so it can be updated from a selection.
        void addInfoRow(QGridLayout* grid, QLabel* value, const QString& caption)
        {
            const int r = grid->rowCount();
            value->setTextInteractionFlags(Qt::TextSelectableByMouse);
            grid->addWidget(value, r, 0, Qt::AlignLeft | Qt::AlignVCenter);
            grid->addWidget(new QLabel(caption), r, 1, Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        // WPF sets Title="{Binding InitialWindowTitle}" and appends TitleExtra; the shell does the same,
        // except that the extra part is only meaningful once a game directory is configured.
        // WPF appends TitleExtra unconditionally because a game directory is always configured by then; here
        // it is only appended once one is, so an unconfigured shell does not show an empty "()".
        // The DirectorySelector host has to be installed BEFORE the view-model is built: its constructor is
        // what asks for a game directory when settings has none (C#'s AvoidEmptyGameDirectory).
        ViewModels::ApplicationViewModel::setDirectorySelectorHandler(
            [this](ViewModels::GameSelectorViewModel* selector, const QString& manualDirectory)
            {
                Views::DirectorySelector dialog(selector, this);
                if (!manualDirectory.isEmpty())
                    dialog.addManualGame(manualDirectory);
                return dialog.exec() == QDialog::Accepted;
            });

        _applicationView = new ViewModels::ApplicationViewModel(this);
        QString title = _applicationView->initialWindowTitle();
        if (Settings::UserSettings::Default()->currentDir() != nullptr)
            title += QStringLiteral(" ") + _applicationView->titleExtra();
        setWindowTitle(title);
        setWindowIcon(QIcon(QStringLiteral(":/Resources/FModel.ico")));
        resize(1280, 800);

        // The window host for MenuCommand's window arms (C#'s Helper.OpenWindow). Two windows are ported;
        // the rest of the arms still report through `deferred` below.
        ViewModels::Commands::MenuCommand::setOpenWindowHandler(
            [this](const QString& window, ViewModels::ApplicationViewModel* contextViewModel)
            {
                if (window == QStringLiteral("Settings"))
                {
                    openSettings(contextViewModel);
                    return true;
                }
                if (window == QStringLiteral("Directory_AES"))
                {
                    openAesManager(contextViewModel);
                    return true;
                }
                return false;
            });

        // Report the menu arms that are still waiting on a port (see onMenuCommand).
        connect(_applicationView->menuCommand(), &ViewModels::Commands::MenuCommand::deferred, this,
                [this](const QString& arm, const QString& waitingOn)
                {
                    log(QStringLiteral("Command '%1' — not yet implemented (waiting on %2).").arg(arm, waitingOn));
                });

        buildMenuBar();

        // Central content: the WPF outer Grid's main row (RootGrid) is a horizontal splitter of the left tab
        // control and the right explorer/editor+log panel.
        auto* central = new QWidget;
        auto* outer = new QVBoxLayout(central);
        outer->setContentsMargins(4, 4, 4, 0);

        auto* rootSplitter = new QSplitter(Qt::Horizontal);
        rootSplitter->setObjectName(QStringLiteral("RootGrid"));
        rootSplitter->addWidget(buildLeftTabControl());
        rootSplitter->addWidget(buildRightPanel());
        rootSplitter->setStretchFactor(0, 0);
        rootSplitter->setStretchFactor(1, 1);
        rootSplitter->setSizes({420, 860});
        outer->addWidget(rootSplitter, 1);

        setCentralWidget(central);
        buildStatusBar();

        // FModel's intro log lines (FLogger), with the [WRN] tag coloured like the real app.
        _logRtbName->append(QStringLiteral(
            "If you're having an issue with FModel, make sure to let us know on Discord at "
            "<a href=\"https://fmodel.app/discord/\" style=\"color:#3d77a8;\">https://fmodel.app/discord/</a>"));
        _logRtbName->append(QString());
        _logRtbName->append(QStringLiteral(
            "<span style=\"color:#D49220;\">[WRN]</span> FModel is free and open-source, if you paid for this, you got scammed"));

        // C#'s Loaded event. Queued so the window is on screen first — the sequence can take seconds on a
        // large install, and (unlike C#) it runs on this thread.
        QMetaObject::invokeMethod(this, &MainWindow::runStartupSequence, Qt::QueuedConnection);
    }

    void MainWindow::buildMenuBar()
    {
        auto* bar = menuBar();

        auto* directory = bar->addMenu(QStringLiteral("Directory"));
        directory->addAction(makeCommandAction(QStringLiteral("Selector"), QStringLiteral("Directory_Selector")));
        directory->addAction(makeCommandAction(QStringLiteral("AES"), QStringLiteral("Directory_AES")));
        directory->addAction(makeCommandAction(QStringLiteral("Backup"), QStringLiteral("Directory_Backup")));
        directory->addAction(makeCommandAction(QStringLiteral("Archives Info"), QStringLiteral("Directory_ArchivesInfo")));

        auto* packages = bar->addMenu(QStringLiteral("Packages"));
        packages->addAction(makeCommandAction(QStringLiteral("Search"), QStringLiteral("Packages_Search"), QStringLiteral("Ctrl+Shift+F")));
        packages->addAction(makeCommandAction(QStringLiteral("References"), QStringLiteral("Packages_References"), QStringLiteral("Ctrl+Shift+R")));
        packages->addAction(makeCommandAction(QStringLiteral("Favorite Directories"), QStringLiteral("Packages_Favorites")));

        auto* views = bar->addMenu(QStringLiteral("Views"));
        views->addAction(makeCommandAction(QStringLiteral("3D Viewer"), QStringLiteral("Views_3dViewer")));
        views->addAction(makeCommandAction(QStringLiteral("Audio Player"), QStringLiteral("Views_AudioPlayer")));
        views->addAction(makeCommandAction(QStringLiteral("Image Merger"), QStringLiteral("Views_ImageMerger")));

        bar->addAction(makeCommandAction(QStringLiteral("Settings"), QStringLiteral("Settings")));

        auto* help = bar->addMenu(QStringLiteral("Help"));
        help->addAction(makeCommandAction(QStringLiteral("Donate"), QStringLiteral("Help_Donate")));
        help->addAction(makeCommandAction(QStringLiteral("Releases"), QStringLiteral("Help_Releases")));
        help->addAction(makeCommandAction(QStringLiteral("Bugs Report"), QStringLiteral("Help_BugsReport")));
        help->addAction(makeCommandAction(QStringLiteral("Discord Server"), QStringLiteral("Help_Discord")));
        help->addAction(makeCommandAction(QStringLiteral("About FModel"), QStringLiteral("Help_About")));

        // Top-bar right side (WPF row 0, column 1): the "Preview New Explorer System" toggle. As a menu-bar corner
        // widget it keeps its "on the right of the top bar" placement.
        auto* corner = new QWidget;
        auto* cornerRow = new QHBoxLayout(corner);
        cornerRow->setContentsMargins(0, 0, 6, 0);
        cornerRow->addWidget(new QLabel(QStringLiteral("Preview New Explorer System")));
        _featurePreviewToggle = new QCheckBox;
        _featurePreviewToggle->setObjectName(QStringLiteral("FeaturePreviewNewAssetExplorer"));
        cornerRow->addWidget(_featurePreviewToggle);
        bar->setCornerWidget(corner, Qt::TopRightCorner);

        // The toggle now routes through ApplicationViewModel.IsAssetsExplorerVisible, which refuses to turn on
        // while the FeaturePreviewNewAssetExplorer setting is off — so the checkbox has to follow the
        // view-model rather than the click, and the stack follows the view-model too.
        connect(_featurePreviewToggle, &QCheckBox::toggled, this, [this](bool on) {
            _applicationView->setIsAssetsExplorerVisible(on);
            const bool visible = _applicationView->isAssetsExplorerVisible();
            if (visible != on)
            {
                QSignalBlocker blocker(_featurePreviewToggle);
                _featurePreviewToggle->setChecked(visible);
                log(QStringLiteral("The new asset explorer is disabled in the settings."));
                return;
            }
            if (_mainStack) _mainStack->setCurrentIndex(visible ? 0 : 1);
            log(visible ? QStringLiteral("Switched to asset explorer view.")
                        : QStringLiteral("Switched to editor tabs view."));
        });
        {
            QSignalBlocker blocker(_featurePreviewToggle);
            _featurePreviewToggle->setChecked(_applicationView->isAssetsExplorerVisible());
        }
    }

    QWidget* MainWindow::buildLeftTabControl()
    {
        _leftTabControl = new QTabWidget;
        _leftTabControl->setObjectName(QStringLiteral("LeftTabControl"));
        _leftTabControl->setMinimumWidth(400);
        connect(_leftTabControl, &QTabWidget::currentChanged, this,
                [this](int index) {
                    // WPF OnTabItemChange also focuses the tab's primary control (no-op for the shell).
                    _applicationView->setSelectedLeftTabIndex(index);
                });

        // --- Tab 0: Archives ---
        {
            auto* page = new QWidget;
            auto* v = new QVBoxLayout(page);

            // "Loading Mode" label + combo, then a full-width Load button.
            auto* modeRow = new QHBoxLayout;
            modeRow->addWidget(new QLabel(QStringLiteral("Loading Mode")));
            modeRow->addSpacing(10);
            _loadingMode = new QComboBox;
            _loadingMode->setObjectName(QStringLiteral("LoadingMode"));
            for (ELoadingMode mode : _applicationView->loadingModes()->modes())
                _loadingMode->addItem(description(mode), QVariant::fromValue<int>(static_cast<int>(mode)));
            _loadingMode->setCurrentIndex(
                _loadingMode->findData(static_cast<int>(Settings::UserSettings::Default()->loadingMode())));
            connect(_loadingMode, &QComboBox::currentIndexChanged, this, [this](int index)
            {
                if (index >= 0)
                    Settings::UserSettings::Default()->setLoadingMode(
                        static_cast<ELoadingMode>(_loadingMode->itemData(index).toInt()));
            });
            modeRow->addWidget(_loadingMode, 1);
            v->addLayout(modeRow);
            auto* load = new QPushButton(QStringLiteral("Load"));
            load->setObjectName(QStringLiteral("LoadButton"));
            connect(load, &QPushButton::clicked, this, &MainWindow::onLoad);
            v->addWidget(load);

            v->addWidget(captionSeparator(QStringLiteral("GAME ARCHIVES")));
            _directoryFilesListBox = new QListWidget;
            _directoryFilesListBox->setObjectName(QStringLiteral("DirectoryFilesListBox"));
            // ELoadingMode::Multiple hands the selection to LoadCommand, so the list is multi-select.
            _directoryFilesListBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
            v->addWidget(_directoryFilesListBox, 1);
            v->addWidget(captionSeparator(QStringLiteral("INFORMATION")));
            auto* info = newInfoGrid();
            _archiveMountPoint = new QLabel(QStringLiteral("/"));
            _archiveFileCount = new QLabel(QStringLiteral("0 Files"));
            _archiveIsEncrypted = new QLabel(QStringLiteral("False"));
            _archiveGuid = new QLabel(QStringLiteral("00000000000000000000000000000000"));
            addInfoRow(info, _archiveMountPoint, QStringLiteral("Mount Point"));
            addInfoRow(info, _archiveFileCount, QStringLiteral("File Count"));
            addInfoRow(info, _archiveIsEncrypted, QStringLiteral("Is Encrypted"));
            addInfoRow(info, _archiveGuid, QStringLiteral("Global Unique Identifier"));
            v->addLayout(info);
            connect(_directoryFilesListBox, &QListWidget::currentRowChanged,
                    this, &MainWindow::onArchiveSelected);
            _leftTabControl->addTab(page, QStringLiteral("Archives"));
        }

        // --- Tab 1: Folders ---
        {
            auto* page = new QWidget;
            auto* v = new QVBoxLayout(page);

            // Summary line ("'None' has 0 folders and 0 packages") + collapse toolbar button.
            auto* summaryRow = new QHBoxLayout;
            auto* labelIcon = new QLabel;
            labelIcon->setPixmap(QIcon(QStringLiteral(":/Resources/label.png")).pixmap(16, 16));
            summaryRow->addWidget(labelIcon);
            summaryRow->addWidget(new QLabel(QStringLiteral("'None' has 0 folders and 0 packages")), 1);
            auto* collapseAll = new QPushButton(QStringLiteral("Collapse"));
            collapseAll->setToolTip(QStringLiteral("Collapse All"));
            connect(collapseAll, &QPushButton::clicked, this, [this] { if (_assetsFolderName) _assetsFolderName->collapseAll(); });
            summaryRow->addWidget(collapseAll, 0);
            v->addLayout(summaryRow);

            _assetsFolderName = new QTreeWidget;
            _assetsFolderName->setObjectName(QStringLiteral("AssetsFolderName"));
            _assetsFolderName->setHeaderHidden(true);
            v->addWidget(_assetsFolderName, 1);
            v->addWidget(captionSeparator(QStringLiteral("INFORMATION")));
            auto* info = newInfoGrid();
            addInfoRow(info, QStringLiteral("0"), QStringLiteral("Packages Count"));
            addInfoRow(info, QStringLiteral("0"), QStringLiteral("Folders Count"));
            addInfoRow(info, QStringLiteral("None"), QStringLiteral("Included In Archive"));
            addInfoRow(info, QStringLiteral("/"), QStringLiteral("Archive Mount Point"));
            addInfoRow(info, QStringLiteral("VER_UE4_LATEST"), QStringLiteral("Archive Version"));
            v->addLayout(info);
            _leftTabControl->addTab(page, QStringLiteral("Folders"));
        }

        // --- Tab 2: Packages (header shows the package count, e.g. "0 Packages") ---
        {
            auto* page = new QWidget;
            auto* v = new QVBoxLayout(page);
            _assetsSearchTextBox = new QLineEdit;
            _assetsSearchTextBox->setObjectName(QStringLiteral("AssetsSearchTextBox"));
            _assetsSearchTextBox->setPlaceholderText(QStringLiteral("Search"));
            v->addWidget(_assetsSearchTextBox);
            auto* breadcrumb = new QLabel(QStringLiteral("No/Directory/Detected/In/Folder"));
            breadcrumb->setStyleSheet(QStringLiteral("color:#9a9aa5;"));
            v->addWidget(breadcrumb);
            _assetsListName = new QListWidget;
            _assetsListName->setObjectName(QStringLiteral("AssetsListName"));
            v->addWidget(_assetsListName, 1);
            v->addWidget(captionSeparator(QStringLiteral("INFORMATION")));
            auto* info = newInfoGrid();
            addInfoRow(info, QStringLiteral("0x0"), QStringLiteral("Offset"));
            addInfoRow(info, QStringLiteral("0 B"), QStringLiteral("Size"));
            addInfoRow(info, QStringLiteral("Unknown"), QStringLiteral("Compression Method"));
            addInfoRow(info, QStringLiteral("False"), QStringLiteral("Is Encrypted"));
            addInfoRow(info, QStringLiteral("None"), QStringLiteral("Included In Archive"));
            v->addLayout(info);
            _leftTabControl->addTab(page, QStringLiteral("0 Packages"));
        }

        return _leftTabControl;
    }

    QWidget* MainWindow::buildRightPanel()
    {
        // WPF: right GroupBox = Grid [ *(main area), Auto(logger expander) ]. As a vertical splitter here.
        auto* rightSplitter = new QSplitter(Qt::Vertical);

        // --- Main area: a stack of the asset explorer and the editor tab control. ---
        _mainStack = new QStackedWidget;

        // Page 0: asset explorer (search + categories, breadcrumb, tiled list).
        {
            auto* explorer = new QWidget;
            auto* v = new QVBoxLayout(explorer);
            auto* topRow = new QHBoxLayout;
            _assetsExplorerSearch = new QLineEdit;
            _assetsExplorerSearch->setObjectName(QStringLiteral("AssetsExplorerSearch"));
            _assetsExplorerSearch->setPlaceholderText(QStringLiteral("Search"));
            _categoriesSelector = new QComboBox;
            _categoriesSelector->setObjectName(QStringLiteral("CategoriesSelector"));
            _categoriesSelector->setMinimumWidth(150);
            // WPF: ItemsSource="{Binding Categories}", rendered through the enum's ToString().
            for (const EAssetCategory category : _applicationView->categories())
                _categoriesSelector->addItem(assetCategoryName(category),
                                             static_cast<uint>(static_cast<uint32_t>(category)));
            topRow->addWidget(_assetsExplorerSearch, 1);
            topRow->addWidget(_categoriesSelector, 0);
            v->addLayout(topRow);
            auto* breadcrumb = new QLabel(QStringLiteral("/"));
            breadcrumb->setObjectName(QStringLiteral("Breadcrumb"));
            v->addWidget(breadcrumb);
            _assetsExplorer = new QListWidget;
            _assetsExplorer->setObjectName(QStringLiteral("AssetsExplorer"));
            _assetsExplorer->setViewMode(QListView::IconMode); // TiledExplorer
            _assetsExplorer->setResizeMode(QListView::Adjust);
            v->addWidget(_assetsExplorer, 1);
            _mainStack->addWidget(explorer);
        }

        // Page 1: editor / viewer tabs ("New Tab" + a "+" add-tab corner button).
        {
            _tabControlName = new QTabWidget;
            _tabControlName->setObjectName(QStringLiteral("TabControlName"));
            _tabControlName->setTabsClosable(true);
            _tabControlName->setMovable(true);
            _tabControlName->addTab(new QWidget, QStringLiteral("New Tab"));
            auto* addTab = new QPushButton(QStringLiteral("+"));
            addTab->setFixedSize(24, 24);
            addTab->setToolTip(QStringLiteral("Add Tab"));
            connect(addTab, &QPushButton::clicked, this, [this] {
                const int i = _tabControlName->addTab(new QWidget, QStringLiteral("New Tab"));
                _tabControlName->setCurrentIndex(i);
            });
            _tabControlName->setCornerWidget(addTab, Qt::TopRightCorner);
            connect(_tabControlName, &QTabWidget::tabCloseRequested, this, [this](int i) {
                if (_tabControlName->count() > 1) _tabControlName->removeTab(i);
            });
            _mainStack->addWidget(_tabControlName);
        }

        // Overlay the two preview toggle buttons (WPF's floating border) at the bottom-right of the main area.
        auto* mainArea = new QWidget;
        auto* overlay = new QGridLayout(mainArea);
        overlay->setContentsMargins(0, 0, 0, 0);
        overlay->addWidget(_mainStack, 0, 0);
        auto* toggles = new QWidget;
        auto* togglesRow = new QHBoxLayout(toggles);
        togglesRow->setContentsMargins(0, 0, 12, 12);
        togglesRow->setSpacing(2);
        auto* explorerToggle = new QPushButton;
        explorerToggle->setIcon(QIcon(QStringLiteral(":/Resources/asset.png")));
        explorerToggle->setToolTip(QStringLiteral("Toggle Asset Explorer"));
        explorerToggle->setFixedSize(32, 32);
        connect(explorerToggle, &QPushButton::clicked, this, [this] {
            if (_mainStack) _mainStack->setCurrentIndex(_mainStack->currentIndex() == 0 ? 1 : 0);
        });
        auto* texturesToggle = new QPushButton;
        texturesToggle->setIcon(QIcon(QStringLiteral(":/Resources/asset_png.png")));
        texturesToggle->setToolTip(QStringLiteral("Preview Textures (OFF)"));
        texturesToggle->setCheckable(true);
        texturesToggle->setFixedSize(32, 32);
        togglesRow->addWidget(explorerToggle);
        togglesRow->addWidget(texturesToggle);
        overlay->addWidget(toggles, 0, 0, Qt::AlignBottom | Qt::AlignRight);
        rightSplitter->addWidget(mainArea);

        // --- Logger: the CustomRichTextBox + the Open-Output / Clear-Logs toolbar buttons (no title, like FModel). ---
        auto* logger = new QWidget;
        auto* logRow = new QHBoxLayout(logger);
        logRow->setContentsMargins(0, 4, 0, 0);
        _logRtbName = new QTextEdit;
        _logRtbName->setObjectName(QStringLiteral("LogRtbName"));
        _logRtbName->setReadOnly(true);
        logRow->addWidget(_logRtbName, 1);
        auto* logButtons = new QVBoxLayout;
        auto* openOutput = new QPushButton;
        openOutput->setIcon(QIcon(QStringLiteral(":/Resources/folder.png")));
        openOutput->setToolTip(QStringLiteral("Open Output Folder"));
        openOutput->setFixedWidth(28);
        connect(openOutput, &QPushButton::clicked, this, [this] { onMenuCommand(QStringLiteral("ToolBox_Open_Output_Directory")); });
        auto* clearLogs = new QPushButton;
        clearLogs->setIcon(QIcon(QStringLiteral(":/Resources/delete.png")));
        clearLogs->setToolTip(QStringLiteral("Clear Logs"));
        clearLogs->setFixedWidth(28);
        connect(clearLogs, &QPushButton::clicked, this, &MainWindow::onClearLogs);
        logButtons->addStretch(1);
        logButtons->addWidget(openOutput);
        logButtons->addWidget(clearLogs);
        logRow->addLayout(logButtons);

        rightSplitter->addWidget(logger);
        rightSplitter->setStretchFactor(0, 1);
        rightSplitter->setStretchFactor(1, 0);
        rightSplitter->setSizes({640, 160});

        _mainStack->setCurrentIndex(1); // default view = editor tabs (matches FModel with nothing loaded)
        return rightSplitter;
    }

    void MainWindow::buildStatusBar()
    {
        auto* bar = statusBar();

        // The status bar is driven by the ApplicationViewModel's FStatus (as in WPF, where the bar binds to
        // Status.Label). The view-model constructor already leaves it Ready.
        _status = _applicationView->status();
        _statusLabel = new QLabel(_status->label());
        _statusLabel->setObjectName(QStringLiteral("StatusLabel"));
        bar->addWidget(_statusLabel);

        // Bind Label -> the QLabel via the ViewModel's PropertyChanged signal.
        connect(_status, &Framework::ViewModel::propertyChanged, this,
                [this](const QString& propertyName)
                {
                    if (propertyName == QStringLiteral("Label"))
                        _statusLabel->setText(_status->label());
                });

        _lastRefreshLabel = new QLabel(QStringLiteral("Last Refresh: never"));
        bar->addPermanentWidget(_lastRefreshLabel);
    }

    QAction* MainWindow::makeCommandAction(const QString& text, const QString& parameter, const QString& shortcut)
    {
        auto* action = new QAction(text, this);
        action->setData(parameter);
        if (!shortcut.isEmpty())
            action->setShortcut(QKeySequence(shortcut));
        connect(action, &QAction::triggered, this, [this, parameter] { onMenuCommand(parameter); });
        return action;
    }

    void MainWindow::onMenuCommand(const QString& parameter)
    {
        // WPF binds every MenuItem to {Binding MenuCommand} with the parameter as CommandParameter; here the
        // QActions carry the same strings and route into the same command. Arms whose target is not ported
        // yet raise `deferred`, reported by the handler wired up in the constructor (the log line used to be
        // unconditional, because there was no command behind it).
        _applicationView->menuCommand()->execute(parameter);
    }

    void MainWindow::openSettings(ViewModels::ApplicationViewModel* contextViewModel)
    {
        // C#'s `new SettingsView().Show()` is modeless, and Helper.OpenWindow re-focuses an already-open one
        // instead of making a second. Neither is ported: the window runs modal here, which gives the same
        // single-instance guarantee without Helper, and a fresh instance each time is correct either way
        // because the view-model re-snapshots UserSettings in its constructor.
        Views::SettingsView view(contextViewModel, this);
        connect(&view, &Views::SettingsView::deferred, this,
                [this](const QString& step, const QString& waitingOn)
                {
                    log(QStringLiteral("Settings step '%1' — not yet implemented (waiting on %2).").arg(step, waitingOn));
                });
        view.exec();
    }

    void MainWindow::openAesManager(ViewModels::ApplicationViewModel* contextViewModel)
    {
        // C#'s AesManager is modeless behind Helper's single-instance guard; modal gives the same guarantee.
        // Closing it is what remounts the provider (see AesManager::closeEvent), so the lists are rebuilt
        // afterwards.
        Views::AesManager view(contextViewModel, this);
        view.exec();
        refreshArchivesList();
    }

    void MainWindow::runStartupSequence()
    {
        // C#'s OnLoaded, in the same order, minus the steps whose subsystems are unported (the update check,
        // the AES/news API calls, Discord, ImGui settings, vgmstream, unluac).
        auto* settings = Settings::UserSettings::Default();
        if (settings->currentDir() == nullptr)
        {
            log(QStringLiteral("No game directory is configured — pick one from Directory > Selector."));
            return;
        }

        // C# downloads these DLLs first (InitOodle / InitZlib / InitDetex). The download half is unported;
        // the loaders are real, and without Oodle most modern paks cannot be decompressed at all.
        const QString dataDir = QDir(settings->outputDirectory()).filePath(QStringLiteral(".data"));
        if (CUE4Parse::Compression::OodleHelper::Initialize(
                QDir(dataDir).filePath(QString::fromLatin1(CUE4Parse::Compression::OodleHelper::OODLE_NAME_CURRENT))
                    .toStdString()) ||
            CUE4Parse::Compression::OodleHelper::Initialize())
        {
            log(QStringLiteral("Oodle loaded."));
        }
        else
        {
            log(QStringLiteral("Oodle could not be loaded — Oodle-compressed archives will fail to read."));
        }
        if (CUE4Parse::Compression::ZstdHelper::Initialize())
            log(QStringLiteral("Zstd loaded."));

        auto* cue4Parse = _applicationView->cue4Parse();
        if (cue4Parse->isUnsupportedLiveService())
        {
            log(QStringLiteral("This entry streams the game from a live service, which is not ported yet. "
                               "Pick a local installation instead."));
            return;
        }

        cue4Parse->initialize();                 // scan the directory, register containers, count loose files
        _applicationView->aesManager()->initAes(); // build the key rows from what settings knows
        _applicationView->updateProvider(true);   // submit the keys, mount, PostMount

        // Deliberate difference from C#: the mappings step is guarded. C# runs it inside Task.Run and the
        // awaiting `async void` swallows the failure at the dispatcher; here an escaping exception would
        // take the process down, and a broken .usmap must not stop a game that has already mounted.
        try
        {
            const QString mappings = cue4Parse->initMappings();
            if (!mappings.isEmpty())
                log(QStringLiteral("Mappings pulled from '%1'").arg(mappings));
        }
        catch (const std::exception& e)
        {
            log(QStringLiteral("Could not read the configured mappings file: %1").arg(QString::fromUtf8(e.what())));
        }

        for (const QString& warning : cue4Parse->verifyConsoleVariables())
            log(warning);

        refreshArchivesList();

        auto* provider = cue4Parse->provider();
        log(QStringLiteral("%1 | Archives: %2 mounted of %3 | AES keys: %4 | Files: %5")
                .arg(_applicationView->gameDisplayName())
                .arg(provider->MountedVfs().size())
                .arg(provider->MountedVfs().size() + provider->UnloadedVfs().size())
                .arg(provider->Keys().size())
                .arg(provider->Files.Count()));
    }

    void MainWindow::refreshArchivesList()
    {
        if (_directoryFilesListBox == nullptr)
            return;

        const QSignalBlocker blocker(_directoryFilesListBox);
        _directoryFilesListBox->clear();

        auto* gameDirectory = _applicationView->cue4Parse()->gameDirectory();
        for (ViewModels::FileItem* item : gameDirectory->directoryFilesView().items())
        {
            auto* row = new QListWidgetItem(item->name(), _directoryFilesListBox);
            row->setData(Qt::UserRole, QVariant::fromValue(item));
            // A container that never mounted (usually a missing AES key) cannot contribute anything.
            if (!item->isEnabled())
                row->setForeground(QColor(QStringLiteral("#8a8a95")));
        }
    }

    void MainWindow::refreshFoldersTree()
    {
        if (_assetsFolderName == nullptr)
            return;

        _assetsFolderName->clear();

        // The tree view-model is fully built by BulkPopulate; this walks it into QTreeWidgetItems. A later
        // slice replaces this with a real model so the tree does not have to be materialised up front.
        std::function<void(ViewModels::TreeItem*, QTreeWidgetItem*)> addNode =
            [&](ViewModels::TreeItem* node, QTreeWidgetItem* parentItem)
        {
            auto* item = parentItem != nullptr ? new QTreeWidgetItem(parentItem)
                                               : new QTreeWidgetItem(_assetsFolderName);
            item->setText(0, node->header());
            item->setData(0, Qt::UserRole, QVariant::fromValue(node));
            for (ViewModels::TreeItem* child : node->foldersView().items())
                addNode(child, item);
        };

        auto* assetsFolder = _applicationView->cue4Parse()->assetsFolder();
        for (ViewModels::TreeItem* root : assetsFolder->foldersView().items())
            addNode(root, nullptr);
    }

    void MainWindow::onLoad()
    {
        // C#: UserSettings.Default.LoadingMode is already set by the combo; the command takes the archive
        // selection, which only ELoadingMode::Multiple reads.
        QVariantList selection;
        for (QListWidgetItem* row : _directoryFilesListBox->selectedItems())
            selection.append(row->data(Qt::UserRole));

        auto* command = _applicationView->loadingModes()->loadCommand();
        const QMetaObject::Connection refused =
            connect(command, &ViewModels::Commands::LoadCommand::refused, this,
                    [this](const QString& reason) { log(reason); });
        const QMetaObject::Connection deferred =
            connect(command, &ViewModels::Commands::LoadCommand::deferred, this,
                    [this](const QString& what, const QString& waitingOn)
                    { log(QStringLiteral("'%1' — not yet implemented (waiting on %2).").arg(what, waitingOn)); });

        command->execute(QVariant(selection));

        disconnect(refused);
        disconnect(deferred);

        refreshFoldersTree();
        _leftTabControl->setCurrentIndex(_applicationView->selectedLeftTabIndex());
    }

    void MainWindow::onArchiveSelected(int row)
    {
        if (row < 0 || _directoryFilesListBox == nullptr)
            return;

        auto* item = _directoryFilesListBox->item(row)->data(Qt::UserRole).value<ViewModels::FileItem*>();
        if (item == nullptr)
            return;

        _archiveMountPoint->setText(item->mountPoint().isEmpty() ? QStringLiteral("/") : item->mountPoint());
        _archiveFileCount->setText(QStringLiteral("%1 Files").arg(item->fileCount()));
        _archiveIsEncrypted->setText(item->isEncrypted() ? QStringLiteral("True") : QStringLiteral("False"));
        _archiveGuid->setText(QString::fromStdString(item->guid().ToString()));
    }

    void MainWindow::onClearLogs()
    {
        if (_logRtbName)
            _logRtbName->clear();
    }

    void MainWindow::log(const QString& message)
    {
        if (!_logRtbName)
            return;
        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
        _logRtbName->append(QStringLiteral("[%1] %2").arg(stamp, message));
    }
}
