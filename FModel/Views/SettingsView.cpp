// Ported from FModel/Views/SettingsView.xaml (+ .xaml.cs)
#include "SettingsView.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "../Extensions/EnumExtensions.h"
#include "../Framework/FStatus.h"
#include "../Framework/Hotkey.h"
#include "../Settings/DirectorySettings.h"
#include "../Settings/EndpointSettings.h"
#include "../Settings/UserSettings.h"
#include "../ViewModels/ApplicationViewModel.h"
#include "../ViewModels/SettingsViewModel.h"
#include "Resources/Controls/DictionaryEditor.h"
#include "Resources/Controls/EndpointEditor.h"

namespace FModel::Views
{
    using Settings::UserSettings;
    using ViewModels::SettingsViewModel;
    using Resources::Controls::DictionaryEditor;
    using Resources::Controls::EndpointEditor;

    namespace
    {
        // The seam described in the header. A function-local static keeps it out of the static-init order.
        std::function<QString(SettingsView::BrowseKind, const QString&)>& browseHandler()
        {
            static std::function<QString(SettingsView::BrowseKind, const QString&)> handler;
            return handler;
        }

        // C#'s EnumToStringConverter, which is GetDescription() on whatever enum the item template is given.
        // The port spells the descriptions as an overload set, but under two names — `description` for the
        // FModel enums, `Description` (returning const char*, so that library stays Qt-free) for the
        // CUE4Parse ones — so each is adapted to one signature here.
        QString describe(EAesReload v) { return description(v); }
        QString describe(EDiscordRpc v) { return description(v); }
        QString describe(ECompressedAudio v) { return description(v); }
        QString describe(EIconStyle v) { return description(v); }
        QString describe(Extensions::Themes::EJsonHighlightTheme v) { return Extensions::Themes::description(v); }
        QString describe(CUE4Parse::UE4::Versions::EGame v) { return Extensions::description(v); }

        template <typename E>
        QString describeNative(E v)
        {
            const char* text = Description(v);
            return text != nullptr ? QString::fromLatin1(text) : QString();
        }

        QString describe(CUE4Parse::UE4::Versions::ELanguage v) { return describeNative(v); }
        QString describe(CUE4Parse::UE4::Assets::Exports::Texture::ETexturePlatform v) { return describeNative(v); }
        QString describe(CUE4Parse::UE4::Assets::Exports::Material::EMaterialFormat v) { return describeNative(v); }
        QString describe(CUE4Parse::UE4::Assets::Exports::Nanite::ENaniteMeshFormat v) { return describeNative(v); }
        QString describe(CUE4Parse_Conversion::Meshes::EMeshFormat v) { return describeNative(v); }
        QString describe(CUE4Parse_Conversion::Meshes::ESocketFormat v) { return describeNative(v); }
        QString describe(CUE4Parse_Conversion::Meshes::ELodFormat v) { return describeNative(v); }
        QString describe(CUE4Parse_Conversion::Textures::ETextureFormat v) { return describeNative(v); }
        QString describe(CUE4Parse_Conversion::UEFormat::Enums::EFileCompressionFormat v) { return describeNative(v); }

        // WPF: `<ComboBox ItemsSource="{Binding X}" SelectedItem="{Binding SelectedX, Mode=TwoWay}">` with the
        // EnumToStringConverter item template. The value rides in the item's data role, so an enum whose
        // members are not consecutive still round-trips.
        template <typename E, typename Apply>
        QComboBox* enumCombo(const QList<E>& values, E current, Apply apply)
        {
            auto* combo = new QComboBox;
            for (const E value : values)
                combo->addItem(describe(value), QVariant::fromValue(static_cast<qulonglong>(value)));

            const int index = static_cast<int>(values.indexOf(current));
            if (index >= 0)
                combo->setCurrentIndex(index);

            QObject::connect(combo, &QComboBox::currentIndexChanged, combo, [combo, values, apply](int i)
            {
                if (i >= 0 && i < static_cast<int>(values.size()))
                    apply(values[i]);
            });
            return combo;
        }

        // The WPF toggle switches. `Content` is the on/off caption BoolToToggleConverter supplies.
        template <typename Apply>
        QCheckBox* toggle(bool checked, Apply apply)
        {
            auto* box = new QCheckBox;
            box->setChecked(checked);
            QObject::connect(box, &QCheckBox::toggled, box, apply);
            return box;
        }

        // A read-only path box plus its "..." button — the shape every output-directory row has.
        QLineEdit* addPathRow(QGridLayout* grid, const QString& caption, const QString& value,
                              SettingsView* receiver, void (SettingsView::*slot)())
        {
            const int row = grid->rowCount();
            grid->addWidget(new QLabel(caption), row, 0);
            auto* box = new QLineEdit(value);
            box->setReadOnly(true);
            grid->addWidget(box, row, 1);
            auto* browse = new QPushButton(QStringLiteral("..."));
            QObject::connect(browse, &QPushButton::clicked, receiver, [receiver, slot] { (receiver->*slot)(); });
            grid->addWidget(browse, row, 2);
            return box;
        }

        // The WPF CustomSeparator: a rule with a caption ("GAME", "ADVANCED", "PREVIEW", ...).
        QWidget* captionSeparator(const QString& caption)
        {
            auto* box = new QWidget;
            auto* row = new QHBoxLayout(box);
            row->setContentsMargins(0, 6, 0, 2);
            auto* line = new QFrame;
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            auto* label = new QLabel(caption);
            label->setStyleSheet(QStringLiteral("color:#9a9aa5;font-size:10px;"));
            row->addWidget(label);
            row->addWidget(line, 1);
            return box;
        }

        int addRow(QGridLayout* grid, const QString& caption, QWidget* control)
        {
            const int row = grid->rowCount();
            grid->addWidget(new QLabel(caption), row, 0);
            grid->addWidget(control, row, 1, 1, 2);
            return row;
        }
    }

    // C# writes this as a raw string literal ("""..."""), where a backslash is NOT an escape — so the JSON
    // really does contain the two characters \n, not a line break. A C++ raw string behaves the same way.
    const QString SettingsView::JsonThemePreviewText = QStringLiteral(R"({
  "title": "This is an example JSON",
  "environment": "production",
  "enabled": true,
  "version": 4,
  "scale": 0.92,
  "features": {
    "previewAssets": true,
    "autoSave": false,
    "maxRecentFiles": 12
  },
  "export": {
    "rootDirectory": "C:\\Exports\\Assets",
    "keepDirectoryStructure": true,
    "formats": [
      "json",
      "png",
      "wav"
    ]
  },
  "paths": [
    "/Game/Characters/Hero",
    "/Game/UI/Widgets",
    "/Game/Audio/Music"
  ],
  "metadata": {
    "lastOpened": "2026-06-20T14:30:00Z",
    "experimental": false,
    "fallbackTheme": null,
    "escapeExample": "Line one\nLine two\tTabbed",
    "accentColor": "#FFC857"
  }
})");

    void SettingsView::setBrowseHandler(std::function<QString(BrowseKind, const QString&)> handler)
    {
        browseHandler() = std::move(handler);
    }

    SettingsView::SettingsView(ViewModels::ApplicationViewModel* applicationView, QWidget* parent)
        : QDialog(parent), _applicationView(applicationView)
    {
        _settingsView = _applicationView->settingsView();
        _settingsView->initialize();

        setWindowTitle(QStringLiteral("Settings"));
        resize(900, 720);

        auto* outer = new QVBoxLayout(this);
        auto* body = new QHBoxLayout;
        outer->addLayout(body, 1);

        _settingsTree = new QTreeWidget;
        _settingsTree->setObjectName(QStringLiteral("SettingsTree"));
        _settingsTree->setHeaderHidden(true);
        _settingsTree->setFixedWidth(180);
        body->addWidget(_settingsTree);

        _pages = new QStackedWidget;
        body->addWidget(_pages, 1);

        // The six DataTemplates, in the tree's order. Each page scrolls: the WPF window is SizeToContent
        // and taller than most screens, which Qt cannot reproduce by growing the dialog.
        const struct { const char* caption; QWidget* (SettingsView::*build)(); } entries[] = {
            {"General",     &SettingsView::buildGeneralPage},
            {"Creator",     &SettingsView::buildCreatorPage},
            {"Models",      &SettingsView::buildModelsPage},
            {"Keybindings", &SettingsView::buildKeybindingsPage},
            {"Unluac",      &SettingsView::buildUnluacPage},
            {"Themes",      &SettingsView::buildThemesPage},
        };

        for (const auto& entry : entries)
        {
            auto* item = new QTreeWidgetItem(_settingsTree, {QString::fromLatin1(entry.caption)});
            _treeItems.append(item);

            auto* scroll = new QScrollArea;
            scroll->setWidgetResizable(true);
            scroll->setWidget((this->*entry.build)());
            _pages->addWidget(scroll);
        }

        _creatorItem = _treeItems[1];
        _unluacItem = _treeItems[4];
        // Both start Collapsed in the WPF style; only Unluac has a trigger this port can evaluate.
        _creatorItem->setHidden(true);
        _unluacItem->setHidden(!UserSettings::Default()->decompileLua());

        connect(_settingsTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current, QTreeWidgetItem*)
        {
            const int index = static_cast<int>(_treeItems.indexOf(current));
            if (index >= 0)
                _pages->setCurrentIndex(index);
            onSelectedItemChanged();
        });

        auto* bottom = new QHBoxLayout;
        auto* note = new QLabel(QStringLiteral("* May Require a restart for changes to take effect"));
        note->setStyleSheet(QStringLiteral("font-size:11px;"));
        bottom->addWidget(note, 1, Qt::AlignRight | Qt::AlignVCenter);
        auto* ok = new QPushButton(QStringLiteral("OK"));
        ok->setDefault(true);
        ok->setMinimumWidth(78);
        connect(ok, &QPushButton::clicked, this, &SettingsView::onClick);
        bottom->addWidget(ok);
        outer->addLayout(bottom);

        selectLastOpenedTab();
    }

    // --- pages -----------------------------------------------------------------------------------------

    QWidget* SettingsView::buildGeneralPage()
    {
        UserSettings* settings = UserSettings::Default();

        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        auto* grid = new QGridLayout;
        grid->setColumnStretch(1, 1);
        layout->addLayout(grid);

        addPathRow(grid, QStringLiteral("Output Directory *"), settings->outputDirectory(),
                   this, &SettingsView::onBrowseOutput);
        addRow(grid, QStringLiteral("Custom Output Folders"),
               toggle(_settingsView->useCustomOutputFolders(),
                      [this](bool on) { _settingsView->setUseCustomOutputFolders(on); }));
        addPathRow(grid, QStringLiteral("Export Raw Data Directory *"),
                   settings->rawDataDirectory(), this, &SettingsView::onBrowseRawData);
        addPathRow(grid, QStringLiteral("Save Properties Directory *"),
                   settings->propertiesDirectory(), this, &SettingsView::onBrowseProperties);
        addPathRow(grid, QStringLiteral("Save Texture Directory *"),
                   settings->textureDirectory(), this, &SettingsView::onBrowseTexture);
        addPathRow(grid, QStringLiteral("Save Audio Directory *"), settings->audioDirectory(),
                   this, &SettingsView::onBrowseAudio);

        addRow(grid, QStringLiteral("Discord Rich Presence"),
               enumCombo(_settingsView->discordRpcs(), _settingsView->selectedDiscordRpc(),
                         [this](EDiscordRpc v) { _settingsView->setSelectedDiscordRpc(v); }));

        layout->addWidget(captionSeparator(QStringLiteral("GAME")));
        auto* game = new QGridLayout;
        game->setColumnStretch(1, 1);
        layout->addLayout(game);

        addPathRow(game, QStringLiteral("Archive Directory *"), settings->gameDirectory(),
                   this, &SettingsView::onBrowseDirectories);
        addRow(game, QStringLiteral("UE Versions *"),
               enumCombo(_settingsView->ueGames(), _settingsView->selectedUeGame(),
                         [this](SettingsViewModel::EGame v) { _settingsView->setSelectedUeGame(v); }));
        addRow(game, QStringLiteral("Texture Platform *"),
               enumCombo(_settingsView->platforms(), _settingsView->selectedUePlatform(),
                         [this](SettingsViewModel::ETexturePlatform v) { _settingsView->setSelectedUePlatform(v); }));
        addRow(game, QStringLiteral("Compressed Audio"),
               enumCombo(_settingsView->compressedAudios(), _settingsView->selectedCompressedAudio(),
                         [this](ECompressedAudio v) { _settingsView->setSelectedCompressedAudio(v); }));
        addRow(game, QStringLiteral("Packages Language"),
               enumCombo(_settingsView->assetLanguages(), _settingsView->selectedAssetLanguage(),
                         [this](SettingsViewModel::ELanguage v) { _settingsView->setSelectedAssetLanguage(v); }));
        addRow(game, QStringLiteral("Keep Directory Structure"),
               toggle(settings->keepDirectoryStructure(),
                      [](bool on) { UserSettings::Default()->setKeepDirectoryStructure(on); }));

        Settings::EndpointSettings* mapping = _settingsView->mappingEndpoint();
        addRow(game, QStringLiteral("Local Mapping File (drag && drop)"),
               toggle(mapping != nullptr && mapping->overwrite(),
                      [this](bool on)
                      {
                          if (Settings::EndpointSettings* m = _settingsView->mappingEndpoint())
                              m->setOverwrite(on);
                      }));
        addPathRow(game, QStringLiteral("Mapping File Path"),
                   mapping != nullptr ? mapping->filePath() : QString(), this, &SettingsView::onBrowseMappings);

        layout->addWidget(captionSeparator(QStringLiteral("ADVANCED")));
        auto* advanced = new QGridLayout;
        advanced->setColumnStretch(1, 1);
        layout->addLayout(advanced);

        auto* versioning = new QWidget;
        auto* versioningRow = new QHBoxLayout(versioning);
        versioningRow->setContentsMargins(0, 0, 0, 0);
        auto* customVersions = new QPushButton(QStringLiteral("Custom Versions"));
        auto* options = new QPushButton(QStringLiteral("Options"));
        auto* mapStructTypes = new QPushButton(QStringLiteral("MapStructTypes"));
        connect(customVersions, &QPushButton::clicked, this, &SettingsView::openCustomVersions);
        connect(options, &QPushButton::clicked, this, &SettingsView::openOptions);
        connect(mapStructTypes, &QPushButton::clicked, this, &SettingsView::openMapStructTypes);
        versioningRow->addWidget(customVersions);
        versioningRow->addWidget(options);
        versioningRow->addWidget(mapStructTypes);
        addRow(advanced, QStringLiteral("Versioning Configuration *"), versioning);

        addRow(advanced, QStringLiteral("AES Reload at Launch"),
               enumCombo(_settingsView->aesReloads(), _settingsView->selectedAesReload(),
                         [this](EAesReload v) { _settingsView->setSelectedAesReload(v); }));

        auto* endpoints = new QWidget;
        auto* endpointRow = new QHBoxLayout(endpoints);
        endpointRow->setContentsMargins(0, 0, 0, 0);
        auto* aesEndpoint = new QPushButton(QStringLiteral("AES"));
        auto* mappingEndpoint = new QPushButton(QStringLiteral("Mapping"));
        connect(aesEndpoint, &QPushButton::clicked, this, &SettingsView::openAesEndpoint);
        connect(mappingEndpoint, &QPushButton::clicked, this, &SettingsView::openMappingEndpoint);
        endpointRow->addWidget(aesEndpoint);
        endpointRow->addWidget(mappingEndpoint);
        addRow(advanced, QStringLiteral("Endpoint Configuration"), endpoints);

        addRow(advanced, QStringLiteral("Serialize Script Bytecode"),
               toggle(settings->readScriptData(),
                      [](bool on) { UserSettings::Default()->setReadScriptData(on); }));
        addRow(advanced, QStringLiteral("Serialize Inlined Shader Maps"),
               toggle(settings->readShaderMaps(),
                      [](bool on) { UserSettings::Default()->setReadShaderMaps(on); }));
        addRow(advanced, QStringLiteral("Decompile Blueprint to Pseudo C++"),
               toggle(settings->showDecompileOption(),
                      [](bool on) { UserSettings::Default()->setShowDecompileOption(on); }));

        // C#'s Checked="OnDecompileLuaChanged" also awaits InitUnluac(); here it only reveals the page (see
        // the header note).
        _decompileLua = toggle(settings->decompileLua(), [this](bool on)
        {
            UserSettings::Default()->setDecompileLua(on);
            if (_unluacItem != nullptr)
                _unluacItem->setHidden(!on);
        });
        addRow(advanced, QStringLiteral("Decompile Lua"), _decompileLua);

        addRow(advanced, QStringLiteral("Convert Audio During Export (.wav)"),
               toggle(settings->convertAudioOnBulkExport(),
                      [](bool on) { UserSettings::Default()->setConvertAudioOnBulkExport(on); }));

        auto* criware = new QLineEdit(QString::number(_settingsView->criwareDecryptionKey()));
        criware->setObjectName(QStringLiteral("CriwareKeyBox"));
        criware->setMaxLength(20);
        criware->setAlignment(Qt::AlignRight);
        criware->setToolTip(QStringLiteral("Enter decryption key in numeric or hexadecimal format "
                                           "(valid key is up to 20 digits or 8 bytes long)"));
        connect(criware, &QLineEdit::textChanged, this, [this](const QString& text)
        {
            // C#'s CriwareKeyBox_TextChanged: empty input is ignored, and a value that does not parse leaves
            // the previous key in place rather than resetting it.
            const QString input = text.trimmed();
            if (input.isEmpty())
                return;

            quint64 parsed = 0;
            if (tryParseKey(input, parsed))
                _settingsView->setCriwareDecryptionKey(parsed);
        });
        addRow(advanced, QStringLiteral("CRIWARE Decryption Key"), criware);

        layout->addStretch(1);
        return page;
    }

    QWidget* SettingsView::buildCreatorPage()
    {
        UserSettings* settings = UserSettings::Default();

        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        auto* grid = new QGridLayout;
        grid->setColumnStretch(1, 1);
        layout->addLayout(grid);

        // The WPF page also previews the selected style as an image (Default.png, NoBackground.png, ...);
        // those resources belong to the icon creator, which is not ported.
        addRow(grid, QStringLiteral("Cosmetic Style"),
               enumCombo(_settingsView->cosmeticStyles(), _settingsView->selectedCosmeticStyle(),
                         [this](EIconStyle v) { _settingsView->setSelectedCosmeticStyle(v); }));
        addRow(grid, QStringLiteral("Cosmetic Shop Icon"),
               toggle(settings->cosmeticDisplayAsset(),
                      [](bool on) { UserSettings::Default()->setCosmeticDisplayAsset(on); }));

        layout->addStretch(1);
        return page;
    }

    QWidget* SettingsView::buildModelsPage()
    {
        UserSettings* settings = UserSettings::Default();

        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        auto* grid = new QGridLayout;
        grid->setColumnStretch(1, 1);
        layout->addLayout(grid);

        addPathRow(grid, QStringLiteral("Model Export Directory *"), settings->modelDirectory(),
                   this, &SettingsView::onBrowseModels);

        addRow(grid, QStringLiteral("Mesh Format"),
               enumCombo(_settingsView->meshExportFormats(), _settingsView->selectedMeshExportFormat(),
                         [this](SettingsViewModel::EMeshFormat v) { _settingsView->setSelectedMeshExportFormat(v); }));

        // WPF greys these two rows through IsEnabled="{Binding SettingsView.SocketSettingsEnabled}" and its
        // compression twin, which the mesh-format setter republishes.
        auto* socketLabel = new QLabel(QStringLiteral("Socket Format (ActorX)"));
        auto* socket = enumCombo(_settingsView->socketExportFormats(), _settingsView->selectedSocketExportFormat(),
                                 [this](SettingsViewModel::ESocketFormat v) { _settingsView->setSelectedSocketExportFormat(v); });
        auto* compressionLabel = new QLabel(QStringLiteral("Compression Format (UEFormat)"));
        auto* compression = enumCombo(_settingsView->compressionFormats(), _settingsView->selectedCompressionFormat(),
                                      [this](SettingsViewModel::EFileCompressionFormat v) { _settingsView->setSelectedCompressionFormat(v); });

        const int socketRow = grid->rowCount();
        grid->addWidget(socketLabel, socketRow, 0);
        grid->addWidget(socket, socketRow, 1, 1, 2);
        const int compressionRow = grid->rowCount();
        grid->addWidget(compressionLabel, compressionRow, 0);
        grid->addWidget(compression, compressionRow, 1, 1, 2);

        auto applyEnables = [this, socketLabel, socket, compressionLabel, compression]
        {
            const bool socketEnabled = _settingsView->socketSettingsEnabled();
            const bool compressionEnabled = _settingsView->compressionSettingsEnabled();
            socketLabel->setEnabled(socketEnabled);
            socket->setEnabled(socketEnabled);
            compressionLabel->setEnabled(compressionEnabled);
            compression->setEnabled(compressionEnabled);
        };
        applyEnables();
        connect(_settingsView, &Framework::ViewModel::propertyChanged, socket,
                [applyEnables](const QString& propertyName)
                {
                    if (propertyName == QStringLiteral("SocketSettingsEnabled") ||
                        propertyName == QStringLiteral("CompressionSettingsEnabled"))
                        applyEnables();
                });

        addRow(grid, QStringLiteral("Level Of Detail Format"),
               enumCombo(_settingsView->lodExportFormats(), _settingsView->selectedLodExportFormat(),
                         [this](SettingsViewModel::ELodFormat v) { _settingsView->setSelectedLodExportFormat(v); }));

        layout->addWidget(captionSeparator(QStringLiteral("PREVIEW")));
        auto* preview = new QGridLayout;
        preview->setColumnStretch(1, 1);
        layout->addLayout(preview);

        auto* maxTextureSize = new QSpinBox;
        maxTextureSize->setRange(0, 16384);
        maxTextureSize->setValue(settings->previewMaxTextureSize());
        connect(maxTextureSize, &QSpinBox::valueChanged, this,
                [](int value) { UserSettings::Default()->setPreviewMaxTextureSize(value); });
        addRow(preview, QStringLiteral("Preview Max Texture Size"), maxTextureSize);

        addRow(preview, QStringLiteral("Preview Static Meshes"),
               toggle(settings->previewStaticMeshes(), [](bool on) { UserSettings::Default()->setPreviewStaticMeshes(on); }));
        addRow(preview, QStringLiteral("Preview Skeletal Meshes"),
               toggle(settings->previewSkeletalMeshes(), [](bool on) { UserSettings::Default()->setPreviewSkeletalMeshes(on); }));
        addRow(preview, QStringLiteral("Preview Animations"),
               toggle(settings->previewAnimations(), [](bool on) { UserSettings::Default()->setPreviewAnimations(on); }));
        addRow(preview, QStringLiteral("Preview Materials"),
               toggle(settings->previewMaterials(), [](bool on) { UserSettings::Default()->setPreviewMaterials(on); }));
        addRow(preview, QStringLiteral("Preview Levels (.umap)"),
               toggle(settings->previewWorlds(), [](bool on) { UserSettings::Default()->setPreviewWorlds(on); }));
        addRow(preview, QStringLiteral("Save Materials Embedded within Meshes"),
               toggle(settings->saveEmbeddedMaterials(), [](bool on) { UserSettings::Default()->setSaveEmbeddedMaterials(on); }));
        addRow(preview, QStringLiteral("Save Morph Targets in Meshes"),
               toggle(settings->saveMorphTargets(), [](bool on) { UserSettings::Default()->setSaveMorphTargets(on); }));
        addRow(preview, QStringLiteral("Handle Skeletons as Empty Meshes"),
               toggle(settings->saveSkeletonAsMesh(), [](bool on) { UserSettings::Default()->setSaveSkeletonAsMesh(on); }));

        auto* formats = new QGridLayout;
        formats->setColumnStretch(1, 1);
        layout->addLayout(formats);
        addRow(formats, QStringLiteral("Nanite Format"),
               enumCombo(_settingsView->naniteMeshExportFormats(), _settingsView->selectedNaniteMeshExportFormat(),
                         [this](SettingsViewModel::ENaniteMeshFormat v) { _settingsView->setSelectedNaniteMeshExportFormat(v); }));
        addRow(formats, QStringLiteral("Material Format"),
               enumCombo(_settingsView->materialExportFormats(), _settingsView->selectedMaterialExportFormat(),
                         [this](SettingsViewModel::EMaterialFormat v) { _settingsView->setSelectedMaterialExportFormat(v); }));
        addRow(formats, QStringLiteral("Texture Format"),
               enumCombo(_settingsView->textureExportFormats(), _settingsView->selectedTextureExportFormat(),
                         [this](SettingsViewModel::ETextureFormat v) { _settingsView->setSelectedTextureExportFormat(v); }));
        addRow(formats, QStringLiteral("Save HDR Textures as Radiance .hdr"),
               toggle(settings->saveHdrTexturesAsHdr(), [](bool on) { UserSettings::Default()->setSaveHdrTexturesAsHdr(on); }));

        layout->addStretch(1);
        return page;
    }

    QWidget* SettingsView::buildKeybindingsPage()
    {
        UserSettings* settings = UserSettings::Default();

        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        auto* grid = new QGridLayout;
        grid->setColumnStretch(1, 1);
        layout->addLayout(grid);

        // WPF uses a HotkeyTextBox, which captures the next keystroke and writes it back. That control is not
        // ported, so each row shows its hotkey read-only — the values themselves are already persisted and
        // acted on by the window's shortcuts.
        const struct { const char* caption; Framework::Hotkey* hotkey; } rows[] = {
            {"Left Switch on Directory Tab",  settings->dirLeftTab()},
            {"Right Switch on Directory Tab", settings->dirRightTab()},
            {"Switch Asset Explorer",         settings->switchAssetExplorer()},
            {"Left Switch on Asset Tab",      settings->assetLeftTab()},
            {"Right Switch on Asset Tab",     settings->assetRightTab()},
            {"Add Asset Tab",                 settings->assetAddTab()},
            {"Remove Selected Asset Tab",     settings->assetRemoveTab()},
            {"Add Audio File",                settings->addAudio()},
            {"Play / Pause Current Audio",    settings->playPauseAudio()},
            {"Previous Audio",                settings->previousAudio()},
            {"Next Audio",                    settings->nextAudio()},
        };

        for (const auto& row : rows)
        {
            auto* box = new QLineEdit(row.hotkey != nullptr ? row.hotkey->toString() : QString());
            box->setReadOnly(true);
            addRow(grid, QString::fromLatin1(row.caption), box);
        }

        layout->addStretch(1);
        return page;
    }

    QWidget* SettingsView::buildUnluacPage()
    {
        using CUE4Parse::UE4::Lua::unluac::EUnluacFlags;
        UserSettings* settings = UserSettings::Default();

        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        auto* grid = new QGridLayout;
        grid->setColumnStretch(1, 1);
        layout->addLayout(grid);

        // The mode combo is spelled out in XAML as two ComboBoxItems rather than bound to a list.
        auto* mode = new QComboBox;
        mode->addItem(QStringLiteral("Decompile"), static_cast<int>(EUnluacMode::Decompile));
        mode->addItem(QStringLiteral("Disassemble"), static_cast<int>(EUnluacMode::Disassemble));
        mode->setCurrentIndex(settings->unluacMode() == EUnluacMode::Disassemble ? 1 : 0);
        connect(mode, &QComboBox::currentIndexChanged, this, [](int index)
        {
            UserSettings::Default()->setUnluacMode(index == 1 ? EUnluacMode::Disassemble : EUnluacMode::Decompile);
        });
        addRow(grid, QStringLiteral("Mode"), mode);

        // C#'s OnUnluacFlagChanged reads the checkbox's Tag, parses it as an EUnluacFlags member and
        // sets/clears that bit. The three tags are RawString, NoDebug and Luaj.
        const struct { const char* caption; EUnluacFlags flag; } flags[] = {
            {"Raw string", EUnluacFlags::RawString},
            {"No debug",   EUnluacFlags::NoDebug},
            {"Luaj",       EUnluacFlags::Luaj},
        };

        for (const auto& entry : flags)
        {
            const EUnluacFlags flag = entry.flag;
            addRow(grid, QString::fromLatin1(entry.caption),
                   toggle(HasFlag(settings->unluacFlags(), flag), [flag](bool on)
                   {
                       UserSettings* current = UserSettings::Default();
                       current->setUnluacFlags(on ? (current->unluacFlags() | flag)
                                                  : (current->unluacFlags() & ~flag));
                   }));
        }

        auto* opcodeMap = new QLineEdit(_settingsView->unluacOpcodeMap());
        connect(opcodeMap, &QLineEdit::textChanged, this,
                [this](const QString& value) { _settingsView->setUnluacOpcodeMap(value); });
        addRow(grid, QStringLiteral("OpcodeMap"), opcodeMap);

        layout->addStretch(1);
        return page;
    }

    QWidget* SettingsView::buildThemesPage()
    {
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        auto* grid = new QGridLayout;
        grid->setColumnStretch(1, 1);
        layout->addLayout(grid);

        addRow(grid, QStringLiteral("JSON Highlight Theme"),
               enumCombo(_settingsView->jsonHighlightThemes(), _settingsView->selectedJsonHighlightTheme(),
                         [this](SettingsViewModel::EJsonHighlightTheme v)
                         { _settingsView->setSelectedJsonHighlightTheme(v); }));

        layout->addWidget(captionSeparator(QStringLiteral("PREVIEW")));
        auto* preview = new QPlainTextEdit(JsonThemePreviewText);
        preview->setObjectName(QStringLiteral("JsonThemePreviewEditor"));
        preview->setReadOnly(true);
        preview->setLineWrapMode(QPlainTextEdit::NoWrap);
        preview->setFont(QFont(QStringLiteral("Cascadia Mono"), 10));
        layout->addWidget(preview, 1);

        return page;
    }

    // --- behaviour -------------------------------------------------------------------------------------

    void SettingsView::selectLastOpenedTab()
    {
        // C#: walk the entries, skipping hidden ones, and select the one whose visible index matches the
        // stored tab. Nothing is selected when the stored index is past the end.
        int i = 0;
        const int target = UserSettings::Default()->lastOpenedSettingTab();
        for (QTreeWidgetItem* item : _treeItems)
        {
            if (item->isHidden())
                continue;
            if (i == target)
            {
                _settingsTree->setCurrentItem(item);
                break;
            }
            i++;
        }
    }

    void SettingsView::onSelectedItemChanged()
    {
        int i = 0;
        for (QTreeWidgetItem* item : _treeItems)
        {
            if (item->isHidden())
                continue;
            if (item != _settingsTree->currentItem())
            {
                i++;
                continue;
            }

            UserSettings::Default()->setLastOpenedSettingTab(i);
            break;
        }
    }

    void SettingsView::onClick()
    {
        QList<SettingsOut> whatShouldIDo;
        const bool restart = _settingsView->save(whatShouldIDo);
        if (restart)
        {
            // contextViewModel.RestartWithWarning()
            emit deferred(QStringLiteral("RestartWithWarning"), QStringLiteral("ViewModels/ApplicationViewModel"));
        }

        accept(); // C#'s Close()

        for (const SettingsOut step : whatShouldIDo)
        {
            switch (step)
            {
                case SettingsOut::ReloadLocres:
                    // Resets LocalizedResourcesCount / LocalResourcesDone / HotfixedResourcesDone and awaits
                    // CUE4Parse.LoadLocalizedResources().
                    emit deferred(QStringLiteral("ReloadLocres"), QStringLiteral("ViewModels/CUE4ParseViewModel"));
                    break;
                case SettingsOut::ReloadMappings:
                    emit deferred(QStringLiteral("ReloadMappings"), QStringLiteral("ViewModels/CUE4ParseViewModel"));
                    break;
            }
        }

        // CUE4Parse.Provider.ReadScriptData / ReadShaderMaps — both belong to the unported provider flags (see
        // AbstractFileProvider.h); the settings themselves are already stored.
        emit deferred(QStringLiteral("Provider.ReadScriptData/ReadShaderMaps"),
                      QStringLiteral("FileProvider/AbstractFileProvider"));

        UserSettings::Save();
    }

    // --- browsing --------------------------------------------------------------------------------------

    bool SettingsView::tryBrowse(BrowseKind kind, const QString& title, QString& path)
    {
        if (const auto& handler = browseHandler())
        {
            path = handler(kind, title);
            return !path.isEmpty();
        }

        path = kind == BrowseKind::Directory
                   ? QFileDialog::getExistingDirectory(nullptr, title)
                   : QFileDialog::getOpenFileName(nullptr, title, QString(),
                                                  QStringLiteral("USMAP Files (*.usmap *.jmap *.jmap.gz);;"
                                                                 "All Files (*.*)"));
        return !path.isEmpty();
    }

    void SettingsView::onBrowseOutput()
    {
        QString path;
        if (!tryBrowse(BrowseKind::Directory, QStringLiteral("Select an output directory"), path)) return;

        UserSettings* settings = UserSettings::Default();
        settings->setOutputDirectory(path);
        if (_settingsView->useCustomOutputFolders()) return;

        // C# appends "Exports" and points the five per-kind directories at it — ModelDirectory is
        // deliberately left alone.
        path = path + QStringLiteral("/Exports");
        settings->setRawDataDirectory(path);
        settings->setPropertiesDirectory(path);
        settings->setTextureDirectory(path);
        settings->setAudioDirectory(path);
        settings->setCodeDirectory(path);
    }

    void SettingsView::onBrowseDirectories()
    {
        QString path;
        if (tryBrowse(BrowseKind::Directory, QStringLiteral("Select an archive directory"), path))
            UserSettings::Default()->setGameDirectory(path);
    }

    void SettingsView::onBrowseRawData()
    {
        QString path;
        if (tryBrowse(BrowseKind::Directory, QStringLiteral("Select a raw data directory"), path))
            UserSettings::Default()->setRawDataDirectory(path);
    }

    void SettingsView::onBrowseProperties()
    {
        QString path;
        if (tryBrowse(BrowseKind::Directory, QStringLiteral("Select a properties directory"), path))
            UserSettings::Default()->setPropertiesDirectory(path);
    }

    void SettingsView::onBrowseTexture()
    {
        QString path;
        if (tryBrowse(BrowseKind::Directory, QStringLiteral("Select a texture directory"), path))
            UserSettings::Default()->setTextureDirectory(path);
    }

    void SettingsView::onBrowseAudio()
    {
        QString path;
        if (tryBrowse(BrowseKind::Directory, QStringLiteral("Select an audio directory"), path))
            UserSettings::Default()->setAudioDirectory(path);
    }

    void SettingsView::onBrowseModels()
    {
        QString path;
        if (tryBrowse(BrowseKind::Directory, QStringLiteral("Select a model directory"), path))
            UserSettings::Default()->setModelDirectory(path);
    }

    void SettingsView::onBrowseMappings()
    {
        QString path;
        if (!tryBrowse(BrowseKind::MappingFile, QStringLiteral("Select a mapping file"), path)) return;
        if (Settings::EndpointSettings* mapping = _settingsView->mappingEndpoint())
            mapping->setFilePath(path);
    }

    // --- the modal editors -----------------------------------------------------------------------------

    bool SettingsView::runConfiguringDialog(QDialog& dialog)
    {
        Framework::FStatus* status = _applicationView->status();
        if (status->isReady())
            status->setStatus(EStatusKind::Configuring);
        const int result = dialog.exec();
        if (status->isReady())
            status->setStatus(EStatusKind::Ready);
        return result == QDialog::Accepted;
    }

    void SettingsView::openCustomVersions()
    {
        DictionaryEditor editor(_settingsView->selectedCustomVersions(), DictionaryEditor::CustomVersionsTitle, this);
        if (!runConfiguringDialog(editor))
            return;
        _settingsView->setSelectedCustomVersions(editor.customVersions());
    }

    void SettingsView::openOptions()
    {
        DictionaryEditor editor(_settingsView->selectedOptions(), DictionaryEditor::OptionsTitle, this);
        if (!runConfiguringDialog(editor))
            return;
        _settingsView->setSelectedOptions(editor.options());
    }

    void SettingsView::openMapStructTypes()
    {
        DictionaryEditor editor(_settingsView->selectedMapStructTypes(), DictionaryEditor::MapStructTypesTitle, this);
        if (!runConfiguringDialog(editor))
            return;
        _settingsView->setSelectedMapStructTypes(editor.mapStructTypes());
    }

    void SettingsView::openAesEndpoint()
    {
        // C# ignores the endpoint editors' result: the endpoint object is edited in place.
        EndpointEditor editor(_settingsView->aesEndpoint(), QStringLiteral("Endpoint Configuration (AES)"),
                              EEndpointType::Aes, this);
        runConfiguringDialog(editor);
    }

    void SettingsView::openMappingEndpoint()
    {
        EndpointEditor editor(_settingsView->mappingEndpoint(),
                              QStringLiteral("Endpoint Configuration (Mapping)"), EEndpointType::Mapping, this);
        runConfiguringDialog(editor);
    }

    // --- the CRIWARE key parser ------------------------------------------------------------------------

    bool SettingsView::tryParseKey(const QString& text, quint64& value)
    {
        value = 0;
        if (text.trimmed().isEmpty())
            return false;

        QString body = text;
        bool isHex = false;
        if (body.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
        {
            isHex = true;
            body = body.mid(2);
        }
        else if (std::any_of(body.cbegin(), body.cend(), [](QChar c) { return c.isLetter(); }))
        {
            isHex = true;
        }

        // Upstream computes `int numberBase = text.All(Uri.IsHexDigit) ? 16 : 10;` and then never uses it —
        // the parse below keys off isHex alone. Kept as a comment rather than as dead code.
        bool ok = false;
        value = body.toULongLong(&ok, isHex ? 16 : 10);
        if (!ok)
            value = 0; // ulong.TryParse zeroes its out parameter on failure.
        return ok;
    }
}
