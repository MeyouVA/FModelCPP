// Behavioural tests for the ported command layer (ViewModels/Commands) and the ApplicationViewModel
// properties that expose it.
//
// The three commands are mostly *dispatch*: a string arrives from a menu item or a context menu and picks a
// branch. That is exactly the part a port gets subtly wrong — a missing arm looks like a dead menu entry, and
// a mis-mapped bulk type writes the right files into the wrong folder — so every arm of all three switches is
// asserted here, including the ones still waiting on an unported view-model.
//
// Nothing here writes to the real clipboard: CopyCommand's line building is tested through buildText, and
// execute() is only driven with the malformed payloads that must leave the clipboard alone.

#include <QtTest>
#include <QClipboard>
#include <QGuiApplication>
#include <QSignalSpy>

#include <stdexcept>

#include "FileProvider/Objects/GameFile.h"

#include "Constants.h"
#include "Enums.h"
#include "Settings/UserSettings.h"
#include "ViewModels/ApplicationViewModel.h"
#include "ViewModels/Commands/CopyCommand.h"
#include "ViewModels/Commands/MenuCommand.h"
#include "ViewModels/Commands/RightClickMenuCommand.h"

using namespace FModel;
using namespace FModel::ViewModels;
using namespace FModel::ViewModels::Commands;
using CUE4Parse::Compression::CompressionMethod;
using CUE4Parse::FileProvider::Objects::FByteBulkDataHeader;
using CUE4Parse::FileProvider::Objects::GameFile;

// A GameFile that exists only for its path — the commands read Path/Name/Directory and nothing else.
class PathOnlyGameFile : public GameFile
{
public:
    explicit PathOnlyGameFile(const std::string& path) : GameFile(path, 0) {}

    bool IsEncrypted() const override { return false; }
    CompressionMethod GetCompressionMethod() const override { return CompressionMethod::None; }
    std::vector<uint8_t> Read(const FByteBulkDataHeader*) override { return {}; }
    std::unique_ptr<CUE4Parse::UE4::Readers::FArchive> CreateReader(const FByteBulkDataHeader*) override
    { return nullptr; }
};

class TestCommands : public QObject
{
    Q_OBJECT

private:
    static QVariant pack(const QString& trigger, const QList<GameFile*>& entries)
    {
        QVariantList selection;
        for (GameFile* entry : entries)
            selection.append(QVariant::fromValue(entry));
        return QVariantList{trigger, selection};
    }

private slots:
    // ---------------------------------------------------------------------------- CopyCommand

    void copyCommandBuildsOneLinePerEntry()
    {
        PathOnlyGameFile a("MyGame/Content/Maps/Level.uasset");
        PathOnlyGameFile b("MyGame/Content/Chars/Hero.uasset");
        const QList<GameFile*> entries{&a, &b};

        QCOMPARE(CopyCommand::buildText(QStringLiteral("File_Path"), entries),
                 QStringLiteral("MyGame/Content/Maps/Level.uasset\r\nMyGame/Content/Chars/Hero.uasset"));
        QCOMPARE(CopyCommand::buildText(QStringLiteral("File_Name"), entries),
                 QStringLiteral("Level.uasset\r\nHero.uasset"));
        QCOMPARE(CopyCommand::buildText(QStringLiteral("Directory_Path"), entries),
                 QStringLiteral("MyGame/Content/Maps\r\nMyGame/Content/Chars"));
        QCOMPARE(CopyCommand::buildText(QStringLiteral("File_Path_No_Extension"), entries),
                 QStringLiteral("MyGame/Content/Maps/Level\r\nMyGame/Content/Chars/Hero"));
        QCOMPARE(CopyCommand::buildText(QStringLiteral("File_Name_No_Extension"), entries),
                 QStringLiteral("Level\r\nHero"));

        // C#'s switch has no default arm, so an unknown trigger copies nothing at all.
        QCOMPARE(CopyCommand::buildText(QStringLiteral("Nope"), entries), QString());

        // One entry: still no trailing newline (TrimEnd).
        QCOMPARE(CopyCommand::buildText(QStringLiteral("File_Name"), {&a}), QStringLiteral("Level.uasset"));
    }

    void copyCommandIgnoresMalformedPayloads()
    {
        ApplicationViewModel vm;
        auto* command = vm.copyCommand();

        // The three early returns: not a list, no selection element, and a selection holding nothing the
        // command recognises. None of them may reach the clipboard.
        const QString before = QGuiApplication::clipboard()->text();

        command->execute(QVariant(QStringLiteral("File_Path")));
        command->execute(QVariant(QVariantList{QStringLiteral("File_Path")}));
        command->execute(QVariant(QVariantList{42, QVariantList{}}));
        command->execute(pack(QStringLiteral("File_Path"), {}));
        command->execute(QVariant(QVariantList{QStringLiteral("File_Path"), QVariantList{QStringLiteral("x")}}));

        QCOMPARE(QGuiApplication::clipboard()->text(), before);
    }

    void applicationViewModelExposesTheCommandsLazily()
    {
        ApplicationViewModel vm;

        // C#'s `??=`: built on first read, the same instance afterwards, and owned by the view-model.
        auto* menu = vm.menuCommand();
        QVERIFY(menu != nullptr);
        QCOMPARE(vm.menuCommand(), menu);
        QCOMPARE(menu->parent(), static_cast<QObject*>(&vm));
        QCOMPARE(menu->contextViewModel(), &vm);

        QVERIFY(vm.copyCommand() != nullptr);
        QCOMPARE(vm.copyCommand(), vm.copyCommand());
        QVERIFY(vm.rightClickMenuCommand() != nullptr);
        QCOMPARE(vm.rightClickMenuCommand(), vm.rightClickMenuCommand());
    }

    // ---------------------------------------------------------------------------- MenuCommand

    void menuCommandOpensExternalTargets()
    {
        QList<QUrl> opened;
        MenuCommand::setOpenUrlHandler([&opened](const QUrl& url) { opened.append(url); return true; });

        ApplicationViewModel vm;
        auto* command = vm.menuCommand();
        QSignalSpy deferredSpy(command, &MenuCommand::deferred);

        Settings::UserSettings::Default()->setOutputDirectory(QStringLiteral("C:/FModel/Output"));

        command->execute(QVariant(QStringLiteral("Help_Donate")));
        command->execute(QVariant(QStringLiteral("Help_BugsReport")));
        command->execute(QVariant(QStringLiteral("Help_Discord")));
        command->execute(QVariant(QStringLiteral("ToolBox_Open_Output_Directory")));

        QCOMPARE(opened.size(), 4);
        QCOMPARE(opened[0], QUrl(Constants::DONATE_LINK));
        QCOMPARE(opened[1], QUrl(Constants::ISSUE_LINK));
        QCOMPARE(opened[2], QUrl(Constants::DISCORD_LINK));
        QCOMPARE(opened[3], QUrl::fromLocalFile(QStringLiteral("C:/FModel/Output")));

        // None of these four is deferred — they are fully ported.
        QCOMPARE(deferredSpy.count(), 0);

        MenuCommand::setOpenUrlHandler({});
    }

    void menuCommandDefersTheUnportedArms()
    {
        QList<QUrl> opened;
        MenuCommand::setOpenUrlHandler([&opened](const QUrl& url) { opened.append(url); return true; });

        ApplicationViewModel vm;
        auto* command = vm.menuCommand();
        QSignalSpy deferredSpy(command, &MenuCommand::deferred);

        // Directory_Selector is NOT here: it is ported, and calls avoidEmptyGameDirectory, which returns
        // without asking when no window host is installed (as in this test).
        const QStringList deferredArms{
            QStringLiteral("Directory_AES"), // ported, but needs a window host — see test_loading
            QStringLiteral("Directory_Backup"),
            QStringLiteral("Directory_ArchivesInfo"),
            QStringLiteral("Views_3dViewer"),
            QStringLiteral("Views_AudioPlayer"),
            QStringLiteral("Views_ImageMerger"),
            QStringLiteral("Settings"),
            QStringLiteral("Help_About"),
            QStringLiteral("Help_Releases"),
            QStringLiteral("ToolBox_Clear_Logs"),
            QStringLiteral("ToolBox_Collapse_All"),
        };

        for (const QString& arm : deferredArms)
            command->execute(QVariant(arm));

        QCOMPARE(deferredSpy.count(), deferredArms.size());
        for (qsizetype i = 0; i < deferredArms.size(); ++i)
        {
            QCOMPARE(deferredSpy[i][0].toString(), deferredArms[i]);
            QVERIFY2(!deferredSpy[i][1].toString().isEmpty(), qPrintable(deferredArms[i]));
        }
        QCOMPARE(opened.size(), 0);

        // "Settings" is in the list above because no window host is installed there; with one, the arm opens
        // the window instead of deferring.
        deferredSpy.clear();
        QStringList openedWindows;
        MenuCommand::setOpenWindowHandler([&openedWindows](const QString& window, ApplicationViewModel*)
        {
            openedWindows.append(window);
            return true;
        });
        command->execute(QVariant(QStringLiteral("Settings")));
        QCOMPARE(openedWindows, QStringList{QStringLiteral("Settings")});
        QCOMPARE(deferredSpy.count(), 0);

        // A host that declines still falls back to the deferred report.
        MenuCommand::setOpenWindowHandler([](const QString&, ApplicationViewModel*) { return false; });
        command->execute(QVariant(QStringLiteral("Settings")));
        QCOMPARE(deferredSpy.count(), 1);
        MenuCommand::setOpenWindowHandler({});

        // A parameter that is not a string is C#'s `case TreeItem selectedFolder:`, which is ported — see
        // test_assets_folder's menuCommandReSelectsATreeItem. Anything else matches no case at all.
        deferredSpy.clear();
        command->execute(QVariant(7));
        QCOMPARE(deferredSpy.count(), 0);

        // An unrecognised string matches no case, and C#'s switch has no default: nothing happens.
        deferredSpy.clear();
        command->execute(QVariant(QStringLiteral("Not_A_Menu_Entry")));
        QCOMPARE(deferredSpy.count(), 0);
        QCOMPARE(opened.size(), 0);

        MenuCommand::setOpenUrlHandler({});
    }

    // ------------------------------------------------------------------- RightClickMenuCommand

    void rightClickTriggersResolve()
    {
        using EAction = RightClickMenuCommand::EAction;
        using EShow = RightClickMenuCommand::EShowAssetType;

        auto check = [](const QString& trigger, EAction action, EShow show, EBulkType bulk)
        {
            const auto resolved = RightClickMenuCommand::resolveTrigger(trigger);
            QVERIFY2(resolved.Action == action, qPrintable(trigger));
            QVERIFY2(resolved.ShowType == show, qPrintable(trigger));
            QVERIFY2(resolved.BulkType == bulk, qPrintable(trigger));
        };

        check(QStringLiteral("Assets_Extract_New_Tab"), EAction::Show, EShow::JSON, EBulkType::None);
        check(QStringLiteral("Assets_Show_Metadata"), EAction::Show, EShow::Metadata, EBulkType::None);
        check(QStringLiteral("Assets_Show_References"), EAction::Show, EShow::References, EBulkType::None);
        // The one "show" trigger that still carries a bulk type — it decompiles into the code directory.
        check(QStringLiteral("Assets_Decompile"), EAction::Show, EShow::Decompile, EBulkType::Code);

        check(QStringLiteral("Save_Data"), EAction::Export, EShow::None, EBulkType::Raw);
        check(QStringLiteral("Save_Properties"), EAction::Export, EShow::None, EBulkType::Properties);
        check(QStringLiteral("Save_Textures"), EAction::Export, EShow::None, EBulkType::Textures);
        check(QStringLiteral("Save_Models"), EAction::Export, EShow::None, EBulkType::Meshes);
        check(QStringLiteral("Save_Animations"), EAction::Export, EShow::None, EBulkType::Animations);
        check(QStringLiteral("Save_Audio"), EAction::Export, EShow::None, EBulkType::Audio);
        check(QStringLiteral("Save_Code"), EAction::Export, EShow::None, EBulkType::Code);

        // C# throws ArgumentOutOfRangeException here rather than falling through silently.
        QVERIFY_EXCEPTION_THROWN(RightClickMenuCommand::resolveTrigger(QStringLiteral("Save_Nothing")),
                                 std::out_of_range);
    }

    void rightClickBulkTargetsReadTheSettings()
    {
        auto* settings = Settings::UserSettings::Default();
        settings->setRawDataDirectory(QStringLiteral("C:/Out/Exports"));
        settings->setPropertiesDirectory(QStringLiteral("C:/Out/JSONs"));
        settings->setTextureDirectory(QStringLiteral("C:/Out/Textures"));
        settings->setModelDirectory(QStringLiteral("C:/Out/Meshes"));
        settings->setAudioDirectory(QStringLiteral("C:/Out/Audios"));
        settings->setCodeDirectory(QStringLiteral("C:/Out/Codes"));

        auto target = [](EBulkType bulk) { return RightClickMenuCommand::resolveBulkTarget(bulk); };

        QCOMPARE(target(EBulkType::Raw).Directory, QStringLiteral("C:/Out/Exports"));
        QCOMPARE(target(EBulkType::Raw).FileType, QStringLiteral("files"));
        QCOMPARE(target(EBulkType::Properties).Directory, QStringLiteral("C:/Out/JSONs"));
        QCOMPARE(target(EBulkType::Properties).FileType, QStringLiteral("json files"));
        QCOMPARE(target(EBulkType::Textures).Directory, QStringLiteral("C:/Out/Textures"));
        QCOMPARE(target(EBulkType::Textures).FileType, QStringLiteral("textures"));
        QCOMPARE(target(EBulkType::Audio).Directory, QStringLiteral("C:/Out/Audios"));
        QCOMPARE(target(EBulkType::Code).Directory, QStringLiteral("C:/Out/Codes"));

        // Meshes and Animations share the model directory but not the word used to log them.
        QCOMPARE(target(EBulkType::Meshes).Directory, QStringLiteral("C:/Out/Meshes"));
        QCOMPARE(target(EBulkType::Animations).Directory, QStringLiteral("C:/Out/Meshes"));
        QCOMPARE(target(EBulkType::Meshes).FileType, QStringLiteral("models"));
        QCOMPARE(target(EBulkType::Animations).FileType, QStringLiteral("animations"));

        // C#'s `_ => (null, null)`, on which the caller returns without exporting anything.
        QVERIFY(target(EBulkType::None).Directory.isEmpty());
        QVERIFY(target(EBulkType::Auto).Directory.isEmpty());
    }

    void rightClickGroupsAssetsByDirectory()
    {
        PathOnlyGameFile a("MyGame/Content/Maps/A.uasset");
        PathOnlyGameFile b("MyGame/Content/Chars/B.uasset");
        PathOnlyGameFile c("MyGame/Content/Maps/C.uasset");

        const auto groups = RightClickMenuCommand::groupAssets({&a, &b, &c}, EBulkType::Textures);

        // GroupBy key order is first-seen, and order within a group is the input order.
        QCOMPARE(groups.size(), 2);
        QCOMPARE(groups[0].Directory, QStringLiteral("MyGame/Content/Maps"));
        QCOMPARE(groups[0].Assets.size(), 2);
        QCOMPARE(groups[0].Assets[0], static_cast<GameFile*>(&a));
        QCOMPARE(groups[0].Assets[1], static_cast<GameFile*>(&c));
        QCOMPARE(groups[1].Directory, QStringLiteral("MyGame/Content/Chars"));
        QCOMPARE(groups[1].Assets.size(), 1);

        // `update` is "more than one asset in this directory", and it is what folds Auto into the bulk type
        // (which is how the extractor knows to batch rather than open a tab per asset).
        QVERIFY(groups[0].Update);
        QVERIFY(!groups[1].Update);
        QVERIFY(groups[0].Bulk == (EBulkType::Textures | EBulkType::Auto));
        QVERIFY(hasFlag(groups[0].Bulk, EBulkType::Auto));
        QVERIFY(groups[1].Bulk == EBulkType::Textures);
        QVERIFY(!hasFlag(groups[1].Bulk, EBulkType::Auto));

        QVERIFY(RightClickMenuCommand::groupAssets({}, EBulkType::Raw).isEmpty());
    }

    void rightClickExportPathFollowsKeepDirectoryStructure()
    {
        auto* settings = Settings::UserSettings::Default();

        settings->setKeepDirectoryStructure(true);
        QCOMPARE(RightClickMenuCommand::exportPath(QStringLiteral("C:/Out/Exports"),
                                                   QStringLiteral("MyGame/Content/Maps")),
                 QStringLiteral("C:/Out/Exports/MyGame/Content/Maps"));

        // Off, only the leaf folder is kept — SubstringAfterLast('/').
        settings->setKeepDirectoryStructure(false);
        QCOMPARE(RightClickMenuCommand::exportPath(QStringLiteral("C:/Out/Exports"),
                                                   QStringLiteral("MyGame/Content/Maps")),
                 QStringLiteral("C:/Out/Exports/Maps"));
        // No '/' at all: SubstringAfterLast returns the whole string.
        QCOMPARE(RightClickMenuCommand::exportPath(QStringLiteral("C:/Out/Exports"), QStringLiteral("Maps")),
                 QStringLiteral("C:/Out/Exports/Maps"));

        // Path.Combine's separator is '\' on Windows, and the whole result is then normalised to '/'.
        settings->setKeepDirectoryStructure(true);
        QCOMPARE(RightClickMenuCommand::exportPath(QStringLiteral("C:\\Out\\Exports"),
                                                   QStringLiteral("MyGame/Content")),
                 QStringLiteral("C:/Out/Exports/MyGame/Content"));
        // A trailing separator is not doubled.
        QCOMPARE(RightClickMenuCommand::exportPath(QStringLiteral("C:/Out/Exports/"),
                                                   QStringLiteral("MyGame")),
                 QStringLiteral("C:/Out/Exports/MyGame"));
    }

    void rightClickExecuteResolvesThenDefers()
    {
        ApplicationViewModel vm;
        auto* command = vm.rightClickMenuCommand();
        QSignalSpy deferredSpy(command, &RightClickMenuCommand::deferred);

        PathOnlyGameFile a("MyGame/Content/Maps/A.uasset");
        PathOnlyGameFile b("MyGame/Content/Maps/B.uasset");

        command->execute(pack(QStringLiteral("Save_Textures"), {&a, &b}));
        QCOMPARE(deferredSpy.count(), 1);
        QCOMPARE(deferredSpy[0][0].toString(), QStringLiteral("Save_Textures"));
        QCOMPARE(deferredSpy[0][1].toInt(), 2);

        // The same early returns CopyCommand has, and none of them may resolve a trigger.
        deferredSpy.clear();
        command->execute(QVariant(QStringLiteral("Save_Textures")));
        command->execute(pack(QStringLiteral("Save_Textures"), {}));
        command->execute(QVariant(QVariantList{42, QVariantList{}}));
        QCOMPARE(deferredSpy.count(), 0);

        // With a real selection, an unknown trigger throws — the resolution happens before any work, so the
        // throw is on the caller's thread in C# too.
        QVERIFY_EXCEPTION_THROWN(command->execute(pack(QStringLiteral("Save_Nothing"), {&a})),
                                 std::out_of_range);
    }
};

QTEST_MAIN(TestCommands)
#include "test_commands.moc"
