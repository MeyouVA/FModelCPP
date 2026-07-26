// Ported from FModel/Views/DirectorySelector.xaml (+ .xaml.cs)
#include "DirectorySelector.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "UE4/Versions/EGame.h"

#include "../Extensions/EnumExtensions.h"
#include "../Helper.h"
#include "../Settings/DirectorySettings.h"
#include "../ViewModels/GameSelectorViewModel.h"

namespace FModel::Views
{
    using CUE4Parse::UE4::Versions::EGame;
    using ViewModels::GameSelectorViewModel;
    using Settings::DirectorySettings;

    namespace
    {
        std::function<QString(const QString&)>& browseHandlerSlot()
        {
            static std::function<QString(const QString&)> handler;
            return handler;
        }

        QString browseForDirectory(QWidget* parent, const QString& caption)
        {
            if (const auto& handler = browseHandlerSlot())
                return handler(caption);
            // Ookii's VistaFolderBrowserDialog { ShowNewFolderButton = false }.
            return QFileDialog::getExistingDirectory(parent, caption, QString(),
                                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        }
    }

    void DirectorySelector::setBrowseHandler(std::function<QString(const QString&)> handler)
    {
        browseHandlerSlot() = std::move(handler);
    }

    DirectorySelector::DirectorySelector(GameSelectorViewModel* gameSelectorViewModel, QWidget* parent)
        : QDialog(parent), _gameSelector(gameSelectorViewModel)
    {
        setWindowTitle(QStringLiteral("Directory Selector"));
        setModal(true);
        resize(560, 460);

        auto* root = new QVBoxLayout(this);

        root->addWidget(new QLabel(QStringLiteral("Select the game you want to extract assets from"), this));

        _detectedDirectories = new QListWidget(this);
        root->addWidget(_detectedDirectories, 1);

        auto* browse = new QPushButton(QStringLiteral("Browse for a game directory..."), this);
        connect(browse, &QPushButton::clicked, this, &DirectorySelector::onBrowseDirectories);
        root->addWidget(browse);

        auto* versionRow = new QGridLayout();
        versionRow->addWidget(new QLabel(QStringLiteral("Unreal Engine Version"), this), 0, 0);
        _ueVersions = new QComboBox(this);
        for (EGame game : _gameSelector->ueGames())
            _ueVersions->addItem(Extensions::description(game), QVariant::fromValue<uint>(game));
        versionRow->addWidget(_ueVersions, 0, 1);
        versionRow->setColumnStretch(1, 1);
        root->addLayout(versionRow);

        connect(_ueVersions, &QComboBox::currentIndexChanged, this, [this](int index)
        {
            if (index < 0 || _gameSelector->selectedDirectory() == nullptr)
                return;
            _gameSelector->selectedDirectory()->setUeVersion(
                static_cast<EGame>(_ueVersions->itemData(index).toUInt()));
        });

        // The XAML's "Add a game manually" expander.
        _manualGameExpander = new QGroupBox(QStringLiteral("Add a game manually"), this);
        _manualGameExpander->setCheckable(true);
        _manualGameExpander->setChecked(false);
        auto* manual = new QGridLayout(_manualGameExpander);
        manual->addWidget(new QLabel(QStringLiteral("Game Name"), _manualGameExpander), 0, 0);
        _helloMyNameIsGame = new QLineEdit(_manualGameExpander);
        manual->addWidget(_helloMyNameIsGame, 0, 1, 1, 2);
        manual->addWidget(new QLabel(QStringLiteral("Paks Directory"), _manualGameExpander), 1, 0);
        _helloGameMyNameIsDirectory = new QLineEdit(_manualGameExpander);
        manual->addWidget(_helloGameMyNameIsDirectory, 1, 1);
        auto* manualBrowse = new QPushButton(QStringLiteral("..."), _manualGameExpander);
        connect(manualBrowse, &QPushButton::clicked, this, &DirectorySelector::onBrowseManualDirectories);
        manual->addWidget(manualBrowse, 1, 2);
        auto* add = new QPushButton(QStringLiteral("Add"), _manualGameExpander);
        connect(add, &QPushButton::clicked, this, &DirectorySelector::onAddDirectory);
        manual->addWidget(add, 2, 2);
        root->addWidget(_manualGameExpander);

        auto* buttons = new QDialogButtonBox(this);
        _deleteButton = buttons->addButton(QStringLiteral("Delete"), QDialogButtonBox::DestructiveRole);
        connect(_deleteButton, &QPushButton::clicked, this, &DirectorySelector::onDeleteDirectory);
        auto* ok = buttons->addButton(QDialogButtonBox::Ok);
        connect(ok, &QPushButton::clicked, this, &DirectorySelector::onClick);
        root->addWidget(buttons);

        connect(_detectedDirectories, &QListWidget::currentRowChanged, this, [this](int row)
        {
            if (row < 0 || row >= _gameSelector->detectedDirectories().size())
                return;
            _gameSelector->setSelectedDirectory(_gameSelector->detectedDirectories()[row]);
            syncFromSelection();
        });

        refreshList();
    }

    void DirectorySelector::refreshList()
    {
        const QSignalBlocker blocker(_detectedDirectories);
        _detectedDirectories->clear();
        int selectedRow = -1;
        const auto& directories = _gameSelector->detectedDirectories();
        for (qsizetype i = 0; i < directories.size(); ++i)
        {
            // The XAML's item template is the game name over its directory.
            _detectedDirectories->addItem(QStringLiteral("%1\n%2")
                                              .arg(directories[i]->gameName(), directories[i]->gameDirectory()));
            if (directories[i] == _gameSelector->selectedDirectory())
                selectedRow = static_cast<int>(i);
        }
        _detectedDirectories->setCurrentRow(selectedRow);
        syncFromSelection();
    }

    void DirectorySelector::syncFromSelection()
    {
        DirectorySettings* selected = _gameSelector->selectedDirectory();
        _deleteButton->setEnabled(selected != nullptr && selected->isManual());
        if (selected == nullptr)
            return;

        const QSignalBlocker blocker(_ueVersions);
        const int index = _ueVersions->findData(QVariant::fromValue<uint>(selected->ueVersion()));
        if (index >= 0)
            _ueVersions->setCurrentIndex(index);
    }

    void DirectorySelector::onBrowseDirectories()
    {
        const QString path = browseForDirectory(this, QStringLiteral("Select a game directory"));
        if (path.isEmpty())
            return;

        _gameSelector->addUndetectedDir(QDir::toNativeSeparators(path));
        refreshList();
    }

    void DirectorySelector::onBrowseManualDirectories()
    {
        const QString path = browseForDirectory(this, QStringLiteral("Select a Paks directory"));
        if (path.isEmpty())
            return;

        const QString native = QDir::toNativeSeparators(path);
        _helloGameMyNameIsDirectory->setText(native);
        _helloMyNameIsGame->setText(Helper::getGameName(path));
    }

    void DirectorySelector::onAddDirectory()
    {
        if (_helloMyNameIsGame->text().isEmpty() || _helloGameMyNameIsDirectory->text().isEmpty())
            return;

        _gameSelector->addUndetectedDir(_helloMyNameIsGame->text(), _helloGameMyNameIsDirectory->text());
        _helloMyNameIsGame->clear();
        _helloGameMyNameIsDirectory->clear();
        refreshList();
    }

    void DirectorySelector::onDeleteDirectory()
    {
        _gameSelector->deleteSelectedGame();
        refreshList();
    }

    void DirectorySelector::onClick()
    {
        accept(); // C#: DialogResult = true; Close();
    }

    void DirectorySelector::addManualGame(const QString& directory)
    {
        _manualGameExpander->setChecked(true);
        _helloMyNameIsGame->setText(Helper::getGameName(directory));
        _helloGameMyNameIsDirectory->setText(directory);
    }
}
