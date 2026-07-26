// Behavioural tests for the Settings window slice: SettingsViewModel, the two JSON/endpoint editors, and the
// SettingsView window itself.
//
// The interesting behaviour here is not the widget tree — it is what the window decides. Save() reports a
// restart and a list of reloads based on snapshot comparisons whose semantics differ between C# and C++ (see
// SettingsViewModel.h), the versioning editors round-trip through the same JSON the settings file uses, and
// the CRIWARE key box parses free-form hex or decimal. Each of those is asserted, plus the two upstream
// quirks the port preserves deliberately, so a later "cleanup" cannot quietly change them.
//
// Nothing here opens a modal window: the browse dialogs go through SettingsView's handler seam, and the
// editors are driven through their text()/onClick() surface.

#include <QtTest>
#include <QSignalSpy>
#include <QTreeWidget>

#include "Enums.h"
#include "Settings/DirectorySettings.h"
#include "Settings/EndpointSettings.h"
#include "Settings/UserSettings.h"
#include "Settings/VersioningSettings.h"
#include "Extensions/EnumExtensions.h"
#include "ViewModels/ApplicationViewModel.h"
#include "ViewModels/Commands/MenuCommand.h"
#include "ViewModels/SettingsViewModel.h"
#include "Views/SettingsView.h"
#include "Views/Resources/Controls/DictionaryEditor.h"
#include "Views/Resources/Controls/EndpointEditor.h"

using namespace FModel;
using namespace FModel::Settings;
using namespace FModel::ViewModels;
using FModel::Views::SettingsView;
using FModel::Views::Resources::Controls::DictionaryEditor;
using FModel::Views::Resources::Controls::EndpointEditor;
using CUE4Parse::UE4::Versions::EGame;
using CUE4Parse::UE4::Versions::ELanguage;

class TestSettingsView : public QObject
{
    Q_OBJECT

private:
    // A UserSettings with a CurrentDir, which is what the settings window expects to find. The C# app cannot
    // reach the window without one (its constructor exits first), so every test here provides it.
    static UserSettings* freshSettings()
    {
        auto* settings = new UserSettings;
        auto* directory = new DirectorySettings(settings);
        directory->setGameName(QStringLiteral("TestGame"));
        directory->setGameDirectory(QStringLiteral("C:/Games/TestGame"));
        directory->setVersioning(new VersioningSettings(directory));
        directory->setEndpoints(EndpointSettings::Default(QStringLiteral("TestGame")));
        settings->setCurrentDir(directory);
        settings->setGameDirectory(directory->gameDirectory());
        UserSettings::SetDefault(settings);
        return settings;
    }

private slots:
    void init() { freshSettings(); }

    // ---------------------------------------------------------------------------- SettingsViewModel

    void initializeSnapshotsTheSettings()
    {
        UserSettings* settings = UserSettings::Default();
        settings->currentDir()->setUeVersion(CUE4Parse::UE4::Versions::GAME_UE5_3);
        settings->currentDir()->setCriwareDecryptionKey(0x1122334455667788ULL);
        settings->setAssetLanguage(ELanguage::French);
        settings->setMeshExportFormat(SettingsViewModel::EMeshFormat::UEFormat);

        SettingsViewModel viewModel;
        viewModel.initialize();

        QCOMPARE(viewModel.selectedUeGame(), CUE4Parse::UE4::Versions::GAME_UE5_3);
        QCOMPARE(viewModel.criwareDecryptionKey(), 0x1122334455667788ULL);
        QCOMPARE(viewModel.selectedAssetLanguage(), ELanguage::French);
        QCOMPARE(viewModel.selectedMeshExportFormat(), SettingsViewModel::EMeshFormat::UEFormat);

        // The two endpoints are the live objects from CurrentDir, not copies.
        QCOMPARE(viewModel.aesEndpoint(), settings->currentDir()->endpoints()[0]);
        QCOMPARE(viewModel.mappingEndpoint(), settings->currentDir()->endpoints()[1]);

        // Every combo source is populated, and the selected value is one of them.
        QVERIFY(!viewModel.ueGames().isEmpty());
        QVERIFY(viewModel.ueGames().contains(CUE4Parse::UE4::Versions::GAME_UE5_3));
        QCOMPARE(viewModel.assetLanguages().size(), 24);
        QCOMPARE(viewModel.platforms().size(), 4);
        QCOMPARE(viewModel.meshExportFormats().size(), 4);
        QCOMPARE(viewModel.jsonHighlightThemes().size(), 16);
    }

    void initializeReproducesTheCompressionFormatSlip()
    {
        // Upstream writes `SelectedCompressionFormat = _selectedCompressionFormat;` — the field, not the
        // snapshot — so the stored value is discarded and the combo opens on the enum's first member.
        UserSettings::Default()->setCompressionFormat(SettingsViewModel::EFileCompressionFormat::GZIP);

        SettingsViewModel viewModel;
        viewModel.initialize();

        QCOMPARE(viewModel.selectedCompressionFormat(), SettingsViewModel::EFileCompressionFormat::None);
    }

    void saveWritesBackAndReportsNothingWhenUnchanged()
    {
        UserSettings* settings = UserSettings::Default();

        SettingsViewModel viewModel;
        viewModel.initialize();

        QList<SettingsOut> whatShouldIDo;
        QCOMPARE(viewModel.save(whatShouldIDo), false);
        QVERIFY(whatShouldIDo.isEmpty());

        // Save() still writes every staged value through, including the one the initialize slip reset.
        QCOMPARE(settings->compressionFormat(), SettingsViewModel::EFileCompressionFormat::None);
    }

    void saveReportsALocresReloadWhenTheLanguageChanges()
    {
        SettingsViewModel viewModel;
        viewModel.initialize();
        viewModel.setSelectedAssetLanguage(ELanguage::Japanese);

        QList<SettingsOut> whatShouldIDo;
        QCOMPARE(viewModel.save(whatShouldIDo), false);
        QCOMPARE(whatShouldIDo, QList<SettingsOut>{SettingsOut::ReloadLocres});
        QCOMPARE(UserSettings::Default()->assetLanguage(), ELanguage::Japanese);
    }

    void saveReportsAMappingsReloadOnlyForOverwriteAndFilePath()
    {
        SettingsViewModel viewModel;
        viewModel.initialize();

        // C#'s hook watches exactly two property names; anything else on the same endpoint is ignored.
        viewModel.mappingEndpoint()->setUrl(QStringLiteral("https://example.invalid/mappings.json"));
        QList<SettingsOut> whatShouldIDo;
        viewModel.save(whatShouldIDo);
        QVERIFY(whatShouldIDo.isEmpty());

        viewModel.mappingEndpoint()->setOverwrite(true);
        viewModel.save(whatShouldIDo);
        QCOMPARE(whatShouldIDo, QList<SettingsOut>{SettingsOut::ReloadMappings});

        // And once raised it never clears, even if the change is undone before OK.
        viewModel.mappingEndpoint()->setOverwrite(false);
        viewModel.save(whatShouldIDo);
        QCOMPARE(whatShouldIDo, QList<SettingsOut>{SettingsOut::ReloadMappings});
    }

    void saveAsksForARestartOnTheVersioningInputs()
    {
        {
            SettingsViewModel viewModel;
            viewModel.initialize();
            viewModel.setSelectedUeGame(CUE4Parse::UE4::Versions::GAME_UE5_4);
            QList<SettingsOut> out;
            QVERIFY(viewModel.save(out));
        }
        {
            SettingsViewModel viewModel;
            viewModel.initialize();
            viewModel.setSelectedUePlatform(SettingsViewModel::ETexturePlatform::NintendoSwitch);
            QList<SettingsOut> out;
            QVERIFY(viewModel.save(out));
        }
        {
            SettingsViewModel viewModel;
            viewModel.initialize();
            UserSettings::Default()->setGameDirectory(QStringLiteral("C:/Games/Other"));
            QList<SettingsOut> out;
            QVERIFY(viewModel.save(out));
        }
        {
            // The reference-comparison quirk: assigning the collections at all counts as a change, even when
            // the contents are identical, because upstream only ever sees a freshly deserialized object here.
            SettingsViewModel viewModel;
            viewModel.initialize();
            viewModel.setSelectedOptions(viewModel.selectedOptions());
            QList<SettingsOut> out;
            QVERIFY(viewModel.save(out));
        }
        {
            // ... but a language change on its own does not need a restart.
            SettingsViewModel viewModel;
            viewModel.initialize();
            viewModel.setSelectedAssetLanguage(ELanguage::German);
            QList<SettingsOut> out;
            QVERIFY(!viewModel.save(out));
        }
    }

    void meshFormatDrivesTheTwoDerivedFlags()
    {
        SettingsViewModel viewModel;
        viewModel.initialize();
        QSignalSpy spy(&viewModel, &Framework::ViewModel::propertyChanged);

        viewModel.setSelectedMeshExportFormat(SettingsViewModel::EMeshFormat::ActorX);
        QVERIFY(viewModel.socketSettingsEnabled());
        QVERIFY(!viewModel.compressionSettingsEnabled());

        viewModel.setSelectedMeshExportFormat(SettingsViewModel::EMeshFormat::UEFormat);
        QVERIFY(!viewModel.socketSettingsEnabled());
        QVERIFY(viewModel.compressionSettingsEnabled());

        // Both derived names are republished on every set, changed or not (C# raises them outside the guard).
        int derived = 0;
        for (const QList<QVariant>& emission : spy)
        {
            const QString name = emission[0].toString();
            if (name == QStringLiteral("SocketSettingsEnabled") ||
                name == QStringLiteral("CompressionSettingsEnabled"))
                derived++;
        }
        QCOMPARE(derived, 4);
    }

    void ueGameListIsDeduplicatedAndReordered()
    {
        const QList<EGame> games = SettingsViewModel::enumerateUeGames();

        // Distinct values only.
        QCOMPARE(QSet<EGame>(games.begin(), games.end()).size(), games.size());

        // Every base engine version comes after every game-specific member.
        qsizetype firstBase = games.size();
        for (qsizetype i = 0; i < games.size(); ++i)
        {
            if ((static_cast<int>(games[i]) & 0xFF) == 0) { firstBase = i; break; }
        }
        QVERIFY(firstBase < games.size());
        for (qsizetype i = firstBase; i < games.size(); ++i)
            QCOMPARE(static_cast<int>(games[i]) & 0xFF, 0);

        // Both halves stay ascending, which is what a stable sort of an ascending list yields.
        for (qsizetype i = 1; i < firstBase; ++i)
            QVERIFY(static_cast<quint32>(games[i - 1]) < static_cast<quint32>(games[i]));
        for (qsizetype i = firstBase + 1; i < games.size(); ++i)
            QVERIFY(static_cast<quint32>(games[i - 1]) < static_cast<quint32>(games[i]));

        QVERIFY(games.contains(CUE4Parse::UE4::Versions::GAME_UE5_6));
        QVERIFY(games.contains(CUE4Parse::UE4::Versions::GAME_ArkSurvivalEvolved));
    }

    void egameDescriptionsFollowTheCSharpFallback()
    {
        // A game names its base version; a base version renders its own decimal value.
        QCOMPARE(Extensions::description(CUE4Parse::UE4::Versions::GAME_ArkSurvivalEvolved),
                 QStringLiteral("GAME_ArkSurvivalEvolved (GAME_UE4_5)"));
        QCOMPARE(Extensions::description(CUE4Parse::UE4::Versions::GAME_UE5_3),
                 QStringLiteral("GAME_UE5_3 (%1)").arg(static_cast<int>(CUE4Parse::UE4::Versions::GAME_UE5_3)));

        // Every member the combo shows must produce a non-empty label with a resolvable suffix.
        for (const EGame game : SettingsViewModel::enumerateUeGames())
        {
            const QString text = Extensions::description(game);
            QVERIFY2(!text.isEmpty(), qPrintable(QString::number(static_cast<int>(game))));
            QVERIFY2(!text.endsWith(QStringLiteral("()")), qPrintable(text));
        }
    }

    // ---------------------------------------------------------------------------- DictionaryEditor

    void dictionaryEditorRoundTripsOptions()
    {
        const QHash<QString, bool> options{{QStringLiteral("HasSkeletalMeshes"), true}};
        DictionaryEditor editor(options, DictionaryEditor::OptionsTitle);

        QVERIFY(editor.text().contains(QStringLiteral("HasSkeletalMeshes")));

        editor.setText(QStringLiteral("{\"A\": true, \"B\": false}"));
        editor.onClick();
        QCOMPARE(editor.result(), int(QDialog::Accepted));
        QCOMPARE(editor.options().size(), 2);
        QCOMPARE(editor.options().value(QStringLiteral("A")), true);
        QCOMPARE(editor.options().value(QStringLiteral("B")), false);
    }

    void dictionaryEditorRoundTripsCustomVersionsAndMapStructTypes()
    {
        {
            DictionaryEditor editor(QList<DictionaryEditor::FCustomVersion>{},
                                    DictionaryEditor::CustomVersionsTitle);
            // An empty collection stands in for C#'s null, so the sample document is shown.
            QVERIFY(editor.text().startsWith(QLatin1Char('[')));
            QVERIFY(editor.text().contains(QStringLiteral("\"Version\"")));

            editor.onClick();
            QCOMPARE(editor.result(), int(QDialog::Accepted));
            QCOMPARE(editor.customVersions().size(), 1);
            QCOMPARE(editor.customVersions()[0].Version, 0);
        }
        {
            DictionaryEditor editor(QHash<QString, DictionaryEditor::MapStructType>{},
                                    DictionaryEditor::MapStructTypesTitle);
            QVERIFY(editor.text().contains(QStringLiteral("MapName")));

            editor.setText(QStringLiteral("{\"Mine\": {\"Key\": \"FName\", \"Value\": \"FVector\"}}"));
            editor.onClick();
            QCOMPARE(editor.mapStructTypes().size(), 1);
            QCOMPARE(editor.mapStructTypes().value(QStringLiteral("Mine")).first, QStringLiteral("FName"));
            QCOMPARE(editor.mapStructTypes().value(QStringLiteral("Mine")).second, QStringLiteral("FVector"));
        }
    }

    void dictionaryEditorRefusesBrokenJson()
    {
        DictionaryEditor editor(QHash<QString, bool>{{QStringLiteral("A"), true}},
                                DictionaryEditor::OptionsTitle);
        editor.setText(QStringLiteral("{ nope"));
        editor.onClick();

        // C# leaves DialogResult unset and the window open; nothing is handed back.
        QVERIFY(editor.isVisible() == false); // never shown, but also never accepted
        QCOMPARE(editor.result(), int(QDialog::Rejected));
        QVERIFY(editor.options().isEmpty());

        // Reset restores the sample document, which then parses.
        editor.onReset();
        editor.onClick();
        QCOMPARE(editor.result(), int(QDialog::Accepted));
        QCOMPARE(editor.options().size(), 2);
    }

    // ---------------------------------------------------------------------------- EndpointEditor

    void endpointEditorEditsInPlaceAndInvalidates()
    {
        SettingsViewModel viewModel;
        viewModel.initialize();
        EndpointSettings* endpoint = viewModel.mappingEndpoint();
        endpoint->setIsValid(true);

        EndpointEditor editor(endpoint, QStringLiteral("Endpoint Configuration (Mapping)"),
                              EEndpointType::Mapping);
        QVERIFY(EndpointEditor::instructions(EEndpointType::Mapping).contains(QStringLiteral("mapping expression")));
        QVERIFY(EndpointEditor::instructions(EEndpointType::Aes).contains(QStringLiteral("AES expression")));

        // OK accepts while the endpoint is valid and was tested (the constructor read IsValid as "tested").
        editor.onClick();
        QCOMPARE(editor.result(), int(QDialog::Accepted));

        // Both request buttons are deferred, and neither may pretend the endpoint has been tested.
        QSignalSpy deferredSpy(&editor, &EndpointEditor::deferred);
        editor.onSend();
        editor.onTest();
        QCOMPARE(deferredSpy.count(), 2);

        // The two documentation buttons go through MenuCommand's URL seam.
        QList<QUrl> opened;
        Commands::MenuCommand::setOpenUrlHandler([&opened](const QUrl& url) { opened.append(url); return true; });
        editor.onSyntax();
        editor.onEvaluator();
        QCOMPARE(opened.size(), 2);
        QCOMPARE(opened[1].toString(), EndpointEditor::EvaluatorLink);
        Commands::MenuCommand::setOpenUrlHandler({});
    }

    // ---------------------------------------------------------------------------- SettingsView

    void windowTreeHidesTheDataDrivenEntries()
    {
        UserSettings::Default()->setDecompileLua(false);

        ApplicationViewModel application;
        SettingsView view(&application);

        auto* tree = view.findChild<QTreeWidget*>(QStringLiteral("SettingsTree"));
        QVERIFY(tree != nullptr);
        QCOMPARE(tree->topLevelItemCount(), 6);
        QCOMPARE(tree->topLevelItem(0)->text(0), QStringLiteral("General"));
        QVERIFY(tree->topLevelItem(1)->isHidden());  // Creator — needs the CUE4Parse provider
        QVERIFY(tree->topLevelItem(4)->isHidden());  // Unluac — DecompileLua is off
        QVERIFY(!tree->topLevelItem(5)->isHidden()); // Themes
    }

    void windowRestoresAndStoresTheOpenedTab()
    {
        UserSettings::Default()->setDecompileLua(false);
        UserSettings::Default()->setLastOpenedSettingTab(2);

        ApplicationViewModel application;
        SettingsView view(&application);
        auto* tree = view.findChild<QTreeWidget*>(QStringLiteral("SettingsTree"));

        // Index 2 counts only the visible entries: General, Models, Keybindings, Themes -> Keybindings.
        QCOMPARE(tree->currentItem()->text(0), QStringLiteral("Keybindings"));

        tree->setCurrentItem(tree->topLevelItem(5));
        QCOMPARE(UserSettings::Default()->lastOpenedSettingTab(), 3);
    }

    void windowBrowseButtonsFanOutTheOutputDirectories()
    {
        SettingsView::setBrowseHandler([](SettingsView::BrowseKind, const QString&)
        { return QStringLiteral("C:/Out"); });

        ApplicationViewModel application;
        SettingsView view(&application);

        view.onBrowseOutput();
        UserSettings* settings = UserSettings::Default();
        QCOMPARE(settings->outputDirectory(), QStringLiteral("C:/Out"));
        QCOMPARE(settings->rawDataDirectory(), QStringLiteral("C:/Out/Exports"));
        QCOMPARE(settings->audioDirectory(), QStringLiteral("C:/Out/Exports"));
        // ModelDirectory is deliberately not part of the fan-out.
        QVERIFY(settings->modelDirectory() != QStringLiteral("C:/Out/Exports"));

        // With custom output folders on, only the root moves.
        application.settingsView()->setUseCustomOutputFolders(true);
        settings->setRawDataDirectory(QStringLiteral("C:/Keep"));
        view.onBrowseOutput();
        QCOMPARE(settings->rawDataDirectory(), QStringLiteral("C:/Keep"));

        // A cancelled dialog changes nothing at all.
        SettingsView::setBrowseHandler([](SettingsView::BrowseKind, const QString&) { return QString(); });
        settings->setGameDirectory(QStringLiteral("C:/Games/TestGame"));
        view.onBrowseDirectories();
        QCOMPARE(settings->gameDirectory(), QStringLiteral("C:/Games/TestGame"));

        SettingsView::setBrowseHandler({});
    }

    void windowOkSavesAndReportsTheUnportedSteps()
    {
        ApplicationViewModel application;
        SettingsView view(&application);
        QSignalSpy deferredSpy(&view, &SettingsView::deferred);

        application.settingsView()->setSelectedAssetLanguage(ELanguage::Korean);
        view.onClick();

        QCOMPARE(view.result(), int(QDialog::Accepted));
        QCOMPARE(UserSettings::Default()->assetLanguage(), ELanguage::Korean);

        QStringList steps;
        for (const QList<QVariant>& emission : deferredSpy)
            steps.append(emission[0].toString());
        QVERIFY(steps.contains(QStringLiteral("ReloadLocres")));
        QVERIFY(steps.contains(QStringLiteral("Provider.ReadScriptData/ReadShaderMaps")));
        QVERIFY(!steps.contains(QStringLiteral("RestartWithWarning")));
    }

    void criwareKeyParsesHexAndDecimal()
    {
        quint64 value = 0;

        QVERIFY(SettingsView::tryParseKey(QStringLiteral("1234567890"), value));
        QCOMPARE(value, 1234567890ULL);

        QVERIFY(SettingsView::tryParseKey(QStringLiteral("0xDEADBEEF"), value));
        QCOMPARE(value, 0xDEADBEEFULL);

        // A bare hex string is detected by the presence of a letter, which is why "0x" is optional.
        QVERIFY(SettingsView::tryParseKey(QStringLiteral("DEADBEEF"), value));
        QCOMPARE(value, 0xDEADBEEFULL);

        // Digits only are read as decimal even when they would also parse as hex.
        QVERIFY(SettingsView::tryParseKey(QStringLiteral("10"), value));
        QCOMPARE(value, 10ULL);

        QVERIFY(!SettingsView::tryParseKey(QString(), value));
        QVERIFY(!SettingsView::tryParseKey(QStringLiteral("   "), value));
        QVERIFY(!SettingsView::tryParseKey(QStringLiteral("nope!"), value));
        QCOMPARE(value, 0ULL);
    }

    void jsonThemePreviewKeepsTheLiteralEscapes()
    {
        // C#'s raw string literal does not process escapes, so these are two characters, not a line break.
        QVERIFY(SettingsView::JsonThemePreviewText.contains(QStringLiteral("Line one\\nLine two\\tTabbed")));
        QVERIFY(SettingsView::JsonThemePreviewText.contains(QStringLiteral("C:\\\\Exports\\\\Assets")));
    }
};

QTEST_MAIN(TestSettingsView)
#include "test_settings_view.moc"
