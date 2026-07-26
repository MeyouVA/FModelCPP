// Behavioural tests for the explorer's folder tree: AssetsFolderViewModel / TreeItem / AssetsListViewModel,
// the GameFileViewModel rows they hold, and the two Framework pieces underneath (RangeObservableCollection
// and the CollectionView standing in for WPF's ICollectionView).
//
// The tree is built by one method — BulkPopulate — that turns a flat list of provider paths into a nested
// structure while every collection is muted, then un-mutes it depth-first. Both halves are easy to port
// *almost* right: a missed suppression makes the views re-materialise once per file, and an off-by-one in
// the path builder gives every node the wrong PathAtThisPoint, which is what the export paths are built
// from. Both are asserted here, along with the search/category filter and the extension resolver.

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>

#include "FileProvider/Objects/GameFile.h"

#include "Enums.h"
#include "Framework/CollectionView.h"
#include "Framework/RangeObservableCollection.h"
#include "Settings/UserSettings.h"
#include "ViewModels/ApplicationViewModel.h"
#include "ViewModels/AssetsFolderViewModel.h"
#include "ViewModels/Commands/MenuCommand.h"
#include "ViewModels/Commands/RightClickMenuCommand.h"

using namespace FModel;
using namespace FModel::ViewModels;
using namespace FModel::ViewModels::Commands;
using CUE4Parse::Compression::CompressionMethod;
using CUE4Parse::FileProvider::Objects::FByteBulkDataHeader;
using CUE4Parse::FileProvider::Objects::GameFile;
using CUE4Parse::UE4::Versions::EGame;
using FModel::Framework::ObservableCollectionBase;
using FModel::Framework::RangeObservableCollection;

// The tree reads Path/Name/Extension and nothing else off an entry.
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

class TestAssetsFolder : public QObject
{
    Q_OBJECT

    // Walks a node path like "MyGame/Content/Maps" from the roots, failing the test if a step is missing.
    static TreeItem* node(AssetsFolderViewModel& tree, const QStringList& path)
    {
        const RangeObservableCollection<TreeItem*>* level = &tree.folders();
        TreeItem* current = nullptr;
        for (const QString& step : path)
        {
            current = nullptr;
            for (TreeItem* item : level->items())
            {
                if (item->header() == step)
                {
                    current = item;
                    break;
                }
            }
            if (current == nullptr)
                return nullptr;
            level = &current->folders();
        }
        return current;
    }

private slots:
    void initTestCase()
    {
        // BulkPopulate strips a %LOCALAPPDATA% prefix; nothing here uses one, but the resolver reads the
        // settings singleton, so it has to exist.
        Settings::UserSettings::SetDefault(new Settings::UserSettings());
        AssetsFolderViewModel::setProjectName(QString());
        GameFileViewModel::setApplicationView(nullptr);
        GameFileViewModel::setGameVersion(std::nullopt);
    }

    // --- Framework/RangeObservableCollection -----------------------------------------------------------

    void rangeCollectionSuppressesAndBatches()
    {
        RangeObservableCollection<int> collection;
        QSignalSpy spy(&collection, &ObservableCollectionBase::collectionChanged);

        collection.add(1);
        collection.add(2);
        QCOMPARE(spy.count(), 2); // one Add each

        spy.clear();
        collection.setSuppressionState(true);
        collection.add(3);
        QCOMPARE(spy.count(), 0);
        QCOMPARE(collection.count(), 3); // muted, but still added

        // AddRange fires exactly one Reset for the whole batch...
        spy.clear();
        collection.addRange({4, 5, 6});
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].value<ObservableCollectionBase::NotifyCollectionChangedAction>(),
                 ObservableCollectionBase::NotifyCollectionChangedAction::Reset);
        QCOMPARE(collection.count(), 6);

        // ...and, per C#, it CLEARS the suppression flag on the way out even though the caller had set it.
        QVERIFY(!collection.suppressionState());
        spy.clear();
        collection.add(7);
        QCOMPARE(spy.count(), 1);
    }

    // --- Framework/CollectionView ----------------------------------------------------------------------

    void collectionViewSortsFiltersAndFollowsSuppression()
    {
        RangeObservableCollection<int> source;
        Framework::CollectionView<int> view(&source, [](int a, int b) { return a < b; });

        source.addRange({5, 1, 4});
        QCOMPARE(view.items(), QList<int>({1, 4, 5}));

        view.setFilter([](int v) { return v != 4; });
        QCOMPARE(view.items(), QList<int>({1, 5}));

        // A muted source does not invalidate the view — the batching BulkPopulate depends on.
        source.setSuppressionState(true);
        source.add(2);
        QCOMPARE(view.items(), QList<int>({1, 5}));

        // Invoking by hand while still muted changes nothing — OnCollectionChanged is where the flag is
        // honoured, so BulkPopulate has to un-mute *before* it invokes, and it does.
        source.invokeOnCollectionChanged();
        QCOMPARE(view.items(), QList<int>({1, 5}));

        source.setSuppressionState(false);
        source.invokeOnCollectionChanged();
        QCOMPARE(view.items(), QList<int>({1, 2, 5}));
    }

    // --- BulkPopulate ----------------------------------------------------------------------------------

    void bulkPopulateBuildsTheNestedTree()
    {
        PathOnlyGameFile a("MyGame/Content/Maps/Level.uasset");
        PathOnlyGameFile b("MyGame/Content/Maps/Other.uasset");
        PathOnlyGameFile c("MyGame/Content/Chars/Hero.uasset");
        PathOnlyGameFile d("MyGame/Plugins/Thing/Data.uasset");

        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a, &b, &c, &d});

        // One root; the second path shares it rather than adding another.
        QCOMPARE(tree.folders().count(), 1);
        QCOMPARE(tree.folders()[0]->header(), QStringLiteral("MyGame"));

        TreeItem* maps = node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content"), QStringLiteral("Maps")});
        QVERIFY(maps != nullptr);
        QCOMPARE(maps->assetsList()->assets().count(), 2);
        QCOMPARE(maps->folders().count(), 0);

        // The file name is NOT part of the tree: only folders[0 .. n-2] become nodes.
        QCOMPARE(maps->pathAtThisPoint(), QStringLiteral("MyGame/Content/Maps"));
        QCOMPARE(node(tree, {QStringLiteral("MyGame")})->pathAtThisPoint(), QStringLiteral("MyGame"));

        TreeItem* content = node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content")});
        QCOMPARE(content->folders().count(), 2); // Maps + Chars
        QCOMPARE(content->assetsList()->assets().count(), 0);

        // Parent links point back up; the root's parent is null.
        QCOMPARE(maps->parentItem(), content);
        QCOMPARE(node(tree, {QStringLiteral("MyGame")})->parentItem(), nullptr);

        QCOMPARE(node(tree, {QStringLiteral("MyGame"), QStringLiteral("Plugins"), QStringLiteral("Thing")})
                     ->assetsList()->assets().count(), 1);

        QCOMPARE(maps->toString(), QStringLiteral("Maps | 0 Folders | 2 Files"));
        QCOMPARE(content->toString(), QStringLiteral("Content | 2 Folders | 0 Files"));
    }

    void bulkPopulateBucketsRootlessFilesUnderContent()
    {
        // A path with no '/' at all has folders.Length == 1 and lands in the synthetic bucket.
        PathOnlyGameFile loose("Loose.uasset");
        PathOnlyGameFile alsoLoose("AlsoLoose.uasset");
        PathOnlyGameFile nested("MyGame/Content/Thing.uasset");

        AssetsFolderViewModel tree;
        tree.bulkPopulate({&loose, &alsoLoose, &nested});

        TreeItem* content = node(tree, {QStringLiteral("Content")});
        QVERIFY(content != nullptr);
        QCOMPARE(content->assetsList()->assets().count(), 2); // both loose files, one bucket
        QCOMPARE(content->pathAtThisPoint(), QStringLiteral("Content"));
        QCOMPARE(content->parentItem(), nullptr);

        // The nested file still builds its own root.
        QVERIFY(node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content")}) != nullptr);
    }

    void bulkPopulateSelectsTheProjectRoot()
    {
        PathOnlyGameFile a("Engine/Content/Thing.uasset");
        PathOnlyGameFile b("MyGame/Content/Thing.uasset");

        // No project name: C#'s `?? treeItems[0]` picks the first root built, which is Engine here.
        {
            AssetsFolderViewModel tree;
            tree.bulkPopulate({&a, &b});
            QVERIFY(node(tree, {QStringLiteral("Engine")})->isSelected());
            QVERIFY(!node(tree, {QStringLiteral("MyGame")})->isSelected());
        }

        // With one, the matching root wins — case-insensitively.
        {
            AssetsFolderViewModel::setProjectName(QStringLiteral("mygame"));
            AssetsFolderViewModel tree;
            tree.bulkPopulate({&a, &b});
            QVERIFY(node(tree, {QStringLiteral("MyGame")})->isSelected());
            QVERIFY(!node(tree, {QStringLiteral("Engine")})->isSelected());
            AssetsFolderViewModel::setProjectName(QString());
        }
    }

    void bulkPopulateEndsWithEveryCollectionUnmuted()
    {
        PathOnlyGameFile a("MyGame/Content/Maps/Level.uasset");
        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a});

        // The recursive InvokeOnCollectionChanged pass has to reach every depth, or the views below the
        // first level never refresh again.
        TreeItem* root = node(tree, {QStringLiteral("MyGame")});
        TreeItem* maps = node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content"), QStringLiteral("Maps")});
        QVERIFY(!root->folders().suppressionState());
        QVERIFY(!maps->folders().suppressionState());
        QVERIFY(!maps->assetsList()->assets().suppressionState());

        // ...and the views can see the contents.
        QCOMPARE(maps->assetsList()->assetsView().count(), 1);
        QCOMPARE(root->foldersView().count(), 1);
    }

    void bulkPopulateIgnoresAnEmptyList()
    {
        AssetsFolderViewModel tree;
        QSignalSpy spy(&tree, &AssetsFolderViewModel::deferred);
        tree.bulkPopulate({});
        QCOMPARE(tree.folders().count(), 0);
        QCOMPARE(spy.count(), 0); // returns before it would have told the search view-model
    }

    void bulkPopulateDefersTheSearchCollection()
    {
        PathOnlyGameFile a("MyGame/Content/Thing.uasset");
        AssetsFolderViewModel tree;
        QSignalSpy spy(&tree, &AssetsFolderViewModel::deferred);
        tree.bulkPopulate({&a});
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy[0][1].toString().contains(QStringLiteral("SearchViewModel")));
    }

    // --- TreeItem views ---------------------------------------------------------------------------------

    void foldersViewSortsByHeader()
    {
        PathOnlyGameFile a("MyGame/Zulu/A.uasset");
        PathOnlyGameFile b("MyGame/Alpha/B.uasset");
        PathOnlyGameFile c("MyGame/Mike/C.uasset");

        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a, &b, &c});

        TreeItem* root = node(tree, {QStringLiteral("MyGame")});
        // Insertion order is Zulu, Alpha, Mike; the view is sorted.
        QCOMPARE(root->folders()[0]->header(), QStringLiteral("Zulu"));
        const QList<TreeItem*> sorted = root->foldersView().items();
        QCOMPARE(sorted.size(), 3);
        QCOMPARE(sorted[0]->header(), QStringLiteral("Alpha"));
        QCOMPARE(sorted[1]->header(), QStringLiteral("Mike"));
        QCOMPARE(sorted[2]->header(), QStringLiteral("Zulu"));
    }

    void searchTextFiltersBothHalves()
    {
        PathOnlyGameFile a("MyGame/Content/HeroSword.uasset");
        PathOnlyGameFile b("MyGame/Content/VillainShield.uasset");
        PathOnlyGameFile c("MyGame/Content/HeroCape/Thing.uasset");
        PathOnlyGameFile d("MyGame/Content/Villains/Thing.uasset");

        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a, &b, &c, &d});
        TreeItem* content = node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content")});

        QCOMPARE(content->assetsList()->assetsView().count(), 2);
        QCOMPARE(content->filteredFoldersView().count(), 2);

        // Case-insensitive, and EVERY whitespace-separated term has to match (C#: filters.All).
        content->setSearchText(QStringLiteral("hero"));
        QCOMPARE(content->assetsList()->assetsView().count(), 1);
        QCOMPARE(content->assetsList()->assetsView().items()[0]->asset()->Name(), std::string("HeroSword.uasset"));
        QCOMPARE(content->filteredFoldersView().count(), 1);
        QCOMPARE(content->filteredFoldersView().items()[0]->header(), QStringLiteral("HeroCape"));

        content->setSearchText(QStringLiteral("  hero   sword  "));
        QCOMPARE(content->assetsList()->assetsView().count(), 1);
        QCOMPARE(content->filteredFoldersView().count(), 0); // no folder matches both terms

        // The unfiltered folder view is unaffected by the search — only FilteredFoldersView carries it.
        QCOMPARE(content->foldersView().count(), 2);

        content->setSearchText(QString());
        QCOMPARE(content->assetsList()->assetsView().count(), 2);
        QCOMPARE(content->filteredFoldersView().count(), 2);
    }

    void categoryFilterHidesFoldersOutrightAndResolvesAssets()
    {
        PathOnlyGameFile sound("MyGame/Content/Voice.wem");
        PathOnlyGameFile text("MyGame/Content/Notes.txt");
        PathOnlyGameFile sub("MyGame/Content/Sub/Thing.txt");

        AssetsFolderViewModel tree;
        tree.bulkPopulate({&sound, &text, &sub});
        TreeItem* content = node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content")});

        // Selecting a category resolves every row's category first, then re-filters.
        content->setSelectedCategory(EAssetCategory::Media);
        QCOMPARE(content->assetsList()->assetsView().count(), 1);
        QCOMPARE(content->assetsList()->assetsView().items()[0]->asset()->Name(), std::string("Voice.wem"));
        // Folders match nothing but "All".
        QCOMPARE(content->filteredFoldersView().count(), 0);

        content->setSelectedCategory(EAssetCategory::Data);
        QCOMPARE(content->assetsList()->assetsView().count(), 1);
        QCOMPARE(content->assetsList()->assetsView().items()[0]->asset()->Name(), std::string("Notes.txt"));

        content->setSelectedCategory(EAssetCategory::All);
        QCOMPARE(content->assetsList()->assetsView().count(), 2);
        QCOMPARE(content->filteredFoldersView().count(), 1);
    }

    void combinedEntriesPutsFoldersBeforeAssets()
    {
        PathOnlyGameFile a("MyGame/Content/Thing.uasset");
        PathOnlyGameFile b("MyGame/Content/Sub/Other.uasset");

        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a, &b});
        TreeItem* content = node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content")});

        const QList<QObject*> combined = content->combinedEntries();
        QCOMPARE(combined.size(), 2);
        QVERIFY(qobject_cast<TreeItem*>(combined[0]) != nullptr);
        QVERIFY(qobject_cast<GameFileViewModel*>(combined[1]) != nullptr);
    }

    // --- GameFileViewModel ------------------------------------------------------------------------------

    void extensionResolverCoversEveryArm()
    {
        struct Case { const char* path; EAssetCategory category; EBulkType actions; };
        const QList<Case> cases{
            {"a/Thing.ini", EAssetCategory::Data, EBulkType::None},
            {"a/Thing.LOCRES", EAssetCategory::Data, EBulkType::None},   // matched case-insensitively
            {"a/Thing.dxbc", EAssetCategory::ByteCode, EBulkType::None},
            {"a/Thing.wem", EAssetCategory::Audio, EBulkType::Audio},
            {"a/Thing.awb", EAssetCategory::Audio, EBulkType::Audio},    // soundbank by nature, audio by choice
            {"a/Thing.acb", EAssetCategory::SoundBank, EBulkType::Audio},
            {"a/Thing.ttf", EAssetCategory::Font, EBulkType::None},
            {"a/Thing.mp4", EAssetCategory::Video, EBulkType::None},
            {"a/Thing.png", EAssetCategory::Texture, EBulkType::Textures},
            {"a/Thing.whatever", EAssetCategory::All, EBulkType::None},  // the default arm
        };

        for (const Case& c : cases)
        {
            PathOnlyGameFile file(c.path);
            GameFileViewModel row(&file);
            QCOMPARE(row.resolvedAssetType(), QString::fromStdString(file.Extension()));
            row.resolve(EResolveCompute::All);
            QVERIFY2(row.assetCategory() == c.category, c.path);
            QVERIFY2(row.assetActions() == c.actions, c.path);
            QVERIFY2(hasFlag(row.resolved(), EResolveCompute::Category), c.path);
        }
    }

    void imageArmReproducesTheResolvedSlip()
    {
        PathOnlyGameFile png("a/Thing.png");
        GameFileViewModel row(&png);
        row.resolve(EResolveCompute::All);

        // Upstream writes `Resolved |= ~EResolveCompute.Preview`, which ORs in every bit except Preview
        // instead of clearing it. The category setter then adds Category back, so Resolved ends at ~0 —
        // and in particular is NOT equal to All, which is what OnIsVisible tests.
        QVERIFY(row.resolved() != EResolveCompute::All);
        QCOMPARE(static_cast<int>(row.resolved()), -1);

        // A non-image row is left at exactly All.
        PathOnlyGameFile txt("a/Thing.txt");
        GameFileViewModel plain(&txt);
        plain.resolve(EResolveCompute::All);
        QCOMPARE(plain.resolved(), EResolveCompute::All);
    }

    void gameSpecificArmsNeedTheGameVersion()
    {
        PathOnlyGameFile ace("a/Thing.ace");

        {
            GameFileViewModel row(&ace);
            row.resolve(EResolveCompute::All);
            QCOMPARE(row.assetCategory(), EAssetCategory::All); // no provider -> default arm
        }

        GameFileViewModel::setGameVersion(EGame::GAME_Borderlands3);
        {
            GameFileViewModel row(&ace);
            row.resolve(EResolveCompute::All);
            QCOMPARE(row.assetCategory(), EAssetCategory::Borderlands);
        }
        {
            // The same extension under a different game falls through to the default.
            GameFileViewModel::setGameVersion(EGame::GAME_Aion2);
            GameFileViewModel row(&ace);
            row.resolve(EResolveCompute::All);
            QCOMPARE(row.assetCategory(), EAssetCategory::All);

            PathOnlyGameFile dat("a/Thing.dat");
            GameFileViewModel datRow(&dat);
            datRow.resolve(EResolveCompute::All);
            QCOMPARE(datRow.assetCategory(), EAssetCategory::Aion2);
        }
        GameFileViewModel::setGameVersion(std::nullopt);
    }

    void packageRowsTakeTheTwoExtensionShortcuts()
    {
        PathOnlyGameFile umap("MyGame/Content/Maps/Level.umap");
        GameFileViewModel world(&umap);
        world.resolve(EResolveCompute::All);
        QCOMPARE(world.assetCategory(), EAssetCategory::World);
        QCOMPARE(world.resolvedAssetType(), QStringLiteral("World"));
        QVERIFY(hasFlag(world.assetActions(), EBulkType::Meshes));
        QVERIFY(hasFlag(world.assetActions(), EBulkType::Code));

        PathOnlyGameFile built("MyGame/Content/Maps/Level_BuiltData.uasset");
        GameFileViewModel buildData(&built);
        buildData.resolve(EResolveCompute::All);
        QCOMPARE(buildData.assetCategory(), EAssetCategory::BuildData);
        QCOMPARE(buildData.resolvedAssetType(), QStringLiteral("MapBuildDataRegistry"));

        // Any other .uasset needs the package resolver, which is not ported.
        PathOnlyGameFile other("MyGame/Content/Thing.uasset");
        GameFileViewModel row(&other);
        QSignalSpy spy(&row, &GameFileViewModel::deferred);
        row.resolve(EResolveCompute::All);
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy[0][1].toString().contains(QStringLiteral("CUE4ParseViewModel")));
        QCOMPARE(row.resolved(), EResolveCompute::All); // marked resolved so the explorer stops retrying
    }

    void resolveIsIdempotentPerComputeBit()
    {
        PathOnlyGameFile txt("a/Thing.txt");
        GameFileViewModel row(&txt);

        row.resolve(EResolveCompute::Category);
        QVERIFY(hasFlag(row.resolved(), EResolveCompute::Category));

        // Everything requested is already resolved, so the second call returns before touching anything.
        // (The extension resolver would otherwise re-run and re-raise its property changes.)
        QSignalSpy spy(&row, &FModel::Framework::ViewModel::propertyChanged);
        row.resolve(EResolveCompute::Category);
        QCOMPARE(spy.count(), 0);
    }

    // --- Command wiring ---------------------------------------------------------------------------------

    void menuCommandReSelectsATreeItem()
    {
        PathOnlyGameFile a("MyGame/Content/Thing.uasset");
        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a});
        TreeItem* root = node(tree, {QStringLiteral("MyGame")});
        root->setIsSelected(true);

        ApplicationViewModel vm;
        auto* command = vm.menuCommand();
        QSignalSpy deferredSpy(command, &MenuCommand::deferred);
        QSignalSpy changedSpy(root, &FModel::Framework::ViewModel::propertyChanged);

        command->execute(QVariant::fromValue(root));

        // false then true — two changes, ending selected, and nothing deferred.
        QCOMPARE(changedSpy.count(), 2);
        QVERIFY(root->isSelected());
        QCOMPARE(deferredSpy.count(), 0);

        // A non-string, non-TreeItem parameter matches no case in C# and does nothing here.
        command->execute(QVariant(7));
        QCOMPARE(deferredSpy.count(), 0);
    }

    void collapseAllWalksTheWholeTree()
    {
        PathOnlyGameFile a("MyGame/Content/Maps/Level.uasset");
        PathOnlyGameFile b("MyGame/Plugins/Thing/Data.uasset");
        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a, &b});

        MenuCommand::setFoldersIsExpanded(&tree, true);
        QVERIFY(node(tree, {QStringLiteral("MyGame")})->isExpanded());
        QVERIFY(node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content"), QStringLiteral("Maps")})
                    ->isExpanded());
        QVERIFY(node(tree, {QStringLiteral("MyGame"), QStringLiteral("Plugins"), QStringLiteral("Thing")})
                    ->isExpanded());

        MenuCommand::setFoldersIsExpanded(&tree, false);
        QVERIFY(!node(tree, {QStringLiteral("MyGame")})->isExpanded());
        QVERIFY(!node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content"), QStringLiteral("Maps")})
                     ->isExpanded());

        // The arm that would call this still has no route to the tree.
        ApplicationViewModel vm;
        QSignalSpy spy(vm.menuCommand(), &MenuCommand::deferred);
        vm.menuCommand()->execute(QVariant(QStringLiteral("ToolBox_Collapse_All")));
        QCOMPARE(spy.count(), 1);
    }

    void rightClickSelectionSplitsFoldersFromAssets()
    {
        PathOnlyGameFile a("MyGame/Content/Thing.uasset");
        PathOnlyGameFile b("MyGame/Content/Other.uasset");
        AssetsFolderViewModel tree;
        tree.bulkPopulate({&a, &b});
        TreeItem* content = node(tree, {QStringLiteral("MyGame"), QStringLiteral("Content")});

        QVariantList selection;
        selection.append(QVariant::fromValue(content));                                  // a folder
        selection.append(QVariant::fromValue(static_cast<GameFile*>(&a)));               // a raw GameFile
        selection.append(QVariant::fromValue(content->assetsList()->assets()[0]));       // a row view-model
        selection.append(QVariant(42));                                                  // C#'s `_ => null`

        const auto split = RightClickMenuCommand::splitSelection(selection);
        QCOMPARE(split.Folders.size(), 1);
        QCOMPARE(split.Folders[0], content);
        QCOMPARE(split.Assets.size(), 2);
        QCOMPARE(split.Assets[0], static_cast<GameFile*>(&a));
        QVERIFY(split.Assets[1] != nullptr);

        // A folder-only selection is still a selection: C# returns only when BOTH halves are empty.
        ApplicationViewModel vm;
        auto* command = vm.rightClickMenuCommand();
        QSignalSpy spy(command, &RightClickMenuCommand::deferred);
        command->execute(QVariantList{QStringLiteral("Save_Textures"),
                                      QVariantList{QVariant::fromValue(content)}});
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][1].toInt(), 0); // no assets, one folder

        // The folder's export path is built from PathAtThisPoint.
        Settings::UserSettings::Default()->setKeepDirectoryStructure(true);
        QCOMPARE(RightClickMenuCommand::exportPath(QStringLiteral("C:/Out/Textures"),
                                                   content->pathAtThisPoint()),
                 QStringLiteral("C:/Out/Textures/MyGame/Content"));
        Settings::UserSettings::Default()->setKeepDirectoryStructure(false);
        QCOMPARE(RightClickMenuCommand::exportPath(QStringLiteral("C:/Out/Textures"),
                                                   content->pathAtThisPoint()),
                 QStringLiteral("C:/Out/Textures/Content"));
    }
};

QTEST_MAIN(TestAssetsFolder)
#include "test_assets_folder.moc"
