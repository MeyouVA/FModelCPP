// Behavioural tests for the ported root view-model (ViewModels/ApplicationViewModel) and the pieces it is
// built from: Extensions/AssetCategoryExtensions, LoadingModesViewModel, Constants, and the EGameName table
// the window title depends on.
//
// The two properties worth testing here are the guarded ones: C# writes both as setters that silently ignore
// out-of-contract values rather than clamping or throwing, and a port that clamped instead would look right
// in the UI while diverging on every rejected assignment.

#include <QtTest>
#include <QSignalSpy>

#include "UE4/Versions/EGame.h"

#include "Constants.h"
#include "Enums.h"
#include "Extensions/AssetCategoryExtensions.h"
#include "Framework/FStatus.h"
#include "Settings/DirectorySettings.h"
#include "Settings/UserSettings.h"
#include "ViewModels/ApplicationViewModel.h"
#include "ViewModels/LoadingModesViewModel.h"

using namespace FModel;
using namespace FModel::Extensions::AssetCategoryExtensions;
using namespace FModel::ViewModels;
using FModel::Framework::ViewModel;
using CUE4Parse::UE4::Versions::EGameName;

class TestApplicationViewModel : public QObject
{
    Q_OBJECT

private slots:
    // ---------------------------------------------------------------- AssetCategoryExtensions

    void categoryBitPacking()
    {
        // A leaf reports its base; a base reports itself.
        QCOMPARE(getBaseCategory(EAssetCategory::StaticMesh), EAssetCategory::Mesh);
        QCOMPARE(getBaseCategory(EAssetCategory::Mesh), EAssetCategory::Mesh);
        QVERIFY(isBaseCategory(EAssetCategory::Mesh));
        QVERIFY(!isBaseCategory(EAssetCategory::StaticMesh));

        // Same base -> of the same category, whichever end you ask from.
        QVERIFY(isOfCategory(EAssetCategory::StaticMesh, EAssetCategory::Mesh));
        QVERIFY(isOfCategory(EAssetCategory::Mesh, EAssetCategory::SkeletalMesh));
        QVERIFY(!isOfCategory(EAssetCategory::StaticMesh, EAssetCategory::Texture));

        // The high half is the category, the low half the leaf index -- Blueprint is deliberately +8, not +7,
        // because the C# enum leaves a gap where a //Metadata comment sits.
        QCOMPARE(static_cast<uint32_t>(EAssetCategory::Blueprint) -
                 static_cast<uint32_t>(EAssetCategory::Blueprints), 8u);
    }

    void baseCategoriesAreExactlyTheBases()
    {
        const auto categories = getBaseCategories();
        QCOMPARE(categories.size(), 11);
        for (const EAssetCategory category : categories)
            QVERIFY2(isBaseCategory(category), qPrintable(assetCategoryName(category)));

        // No duplicates, and the list is in declaration order.
        for (int i = 1; i < categories.size(); ++i)
            QVERIFY(static_cast<uint32_t>(categories[i]) > static_cast<uint32_t>(categories[i - 1]));

        QCOMPARE(categories.first(), EAssetCategory::All);
        QCOMPARE(categories.last(), EAssetCategory::GameSpecific);
    }

    // ---------------------------------------------------------------- LoadingModesViewModel

    void loadingModesListsEveryMode()
    {
        LoadingModesViewModel vm;
        QCOMPARE(vm.modes().size(), 5);
        QCOMPARE(vm.modes().first(), ELoadingMode::Multiple);
        QCOMPARE(vm.modes().last(), ELoadingMode::AllButPatched);
        // Every mode has a description (the picker renders these).
        for (const ELoadingMode mode : vm.modes())
            QVERIFY(!description(mode).isEmpty());
    }

    // ---------------------------------------------------------------- ApplicationViewModel

    void constructorLeavesTheAppReady()
    {
        ApplicationViewModel vm;
        QVERIFY(vm.status() != nullptr);
        // FStatus starts in Loading; the constructor's last act is to mark it Ready.
        QCOMPARE(vm.status()->kind(), EStatusKind::Ready);
        QVERIFY(vm.status()->isReady());
        QVERIFY(vm.loadingModes() != nullptr);
        QCOMPARE(vm.categories(), getBaseCategories());
    }

    void buildIsResolvedFromTheBuildType()
    {
#ifdef NDEBUG
        const EBuildKind expected = EBuildKind::Release;
#else
        const EBuildKind expected = EBuildKind::Debug;
#endif
        ApplicationViewModel vm;
        QCOMPARE(vm.build(), expected);
        // Unknown is only reachable in C# when neither DEBUG nor RELEASE is defined, so it must never be
        // what a real build reports.
        QVERIFY(vm.build() != EBuildKind::Unknown);
    }

    void selectedLeftTabIndexIgnoresOutOfRange()
    {
        ApplicationViewModel vm;
        QSignalSpy spy(&vm, &ViewModel::propertyChanged);

        vm.setSelectedLeftTabIndex(2);
        QCOMPARE(vm.selectedLeftTabIndex(), 2);
        QCOMPARE(spy.count(), 1);

        // Out of range is *ignored*, not clamped -- the index keeps its previous value and nothing is raised.
        vm.setSelectedLeftTabIndex(3);
        QCOMPARE(vm.selectedLeftTabIndex(), 2);
        vm.setSelectedLeftTabIndex(-1);
        QCOMPARE(vm.selectedLeftTabIndex(), 2);
        QCOMPARE(spy.count(), 1);

        // Assigning the value it already holds raises nothing either (SetProperty's equality check).
        vm.setSelectedLeftTabIndex(2);
        QCOMPARE(spy.count(), 1);

        vm.setSelectedLeftTabIndex(0);
        QCOMPARE(vm.selectedLeftTabIndex(), 0);
        QCOMPARE(spy.count(), 2);
    }

    void assetsExplorerVisibilityFollowsTheFeatureFlag()
    {
        auto* settings = Settings::UserSettings::Default();
        const bool previous = settings->featurePreviewNewAssetExplorer();

        ApplicationViewModel vm;

        settings->setFeaturePreviewNewAssetExplorer(false);
        vm.setIsAssetsExplorerVisible(true);
        QVERIFY2(!vm.isAssetsExplorerVisible(), "turning it on must be refused while the feature is off");

        settings->setFeaturePreviewNewAssetExplorer(true);
        vm.setIsAssetsExplorerVisible(true);
        QVERIFY(vm.isAssetsExplorerVisible());

        // Turning it *off* is never gated -- only the true branch consults the setting.
        settings->setFeaturePreviewNewAssetExplorer(false);
        vm.setIsAssetsExplorerVisible(false);
        QVERIFY(!vm.isAssetsExplorerVisible());

        settings->setFeaturePreviewNewAssetExplorer(previous);
    }

    void titleStrings()
    {
        ApplicationViewModel vm;

        const QString title = vm.initialWindowTitle();
        QVERIFY(title.startsWith(QStringLiteral("FModel (")));
        QVERIFY(title.endsWith(QLatin1Char(')')));
        QVERIFY(title.contains(Constants::APP_SHORT_COMMIT_ID()));
        QVERIFY(Constants::APP_SHORT_COMMIT_ID().size() <= 7);

        // With no game configured C# would have exited; the port renders an empty version instead.
        auto* settings = Settings::UserSettings::Default();
        auto* previous = settings->currentDir();
        settings->setCurrentDir(nullptr);
        const QString bare = vm.titleExtra();
        QVERIFY(bare.startsWith(QStringLiteral("()")));

        Settings::DirectorySettings dir;
        dir.setUeVersion(CUE4Parse::UE4::Versions::GAME_UE5_3);
        settings->setCurrentDir(&dir);
        QVERIFY(vm.titleExtra().startsWith(QStringLiteral("(GAME_UE5_3)")));
        // Debug builds append the build kind; release builds append nothing.
        if (vm.build() == EBuildKind::Release)
            QCOMPARE(vm.titleExtra(), QStringLiteral("(GAME_UE5_3)"));
        else
            QCOMPARE(vm.titleExtra(), QStringLiteral("(GAME_UE5_3) (%1)").arg(buildKindName(vm.build())));

        settings->setCurrentDir(previous);
    }

    void gameDisplayNameIsUnknownWithoutAProvider()
    {
        ApplicationViewModel vm;
        QCOMPARE(vm.gameDisplayName(), QStringLiteral("Unknown"));
    }

    // ---------------------------------------------------------------- EGameName

    void egameNamesAndAliases()
    {
        QCOMPARE(QLatin1String(EGameName(CUE4Parse::UE4::Versions::GAME_UE4_27)), QLatin1String("GAME_UE4_27"));
        QCOMPARE(QLatin1String(EGameName(CUE4Parse::UE4::Versions::GAME_UE5_3)), QLatin1String("GAME_UE5_3"));
        // A game-specific member, not just the engine rungs.
        QCOMPARE(QLatin1String(EGameName(CUE4Parse::UE4::Versions::GAME_SeaOfThieves)),
                 QLatin1String("GAME_SeaOfThieves"));
        // The three alias members report the first-declared name at their value, as .NET does.
        QCOMPARE(QLatin1String(EGameName(CUE4Parse::UE4::Versions::GAME_UE5_LATEST)),
                 QLatin1String(EGameName(CUE4Parse::UE4::Versions::GAME_UE5_9)));
        // A value that is not a declared member has no name (C# would render the number).
        QCOMPARE(EGameName(static_cast<CUE4Parse::UE4::Versions::EGame>(0xDEADBEEF)), nullptr);
    }

    // ---------------------------------------------------------------- Constants

    void constantsCarryOverFromCSharp()
    {
        QCOMPARE(Constants::ZERO_64_CHAR.size(), 64);
        QVERIFY(!Constants::ZERO_GUID.IsValid());
        QCOMPARE(Constants::PALETTE_LENGTH(), 10);
        QCOMPARE(Constants::PALETTE_LENGTH(), static_cast<int>(Constants::COLOR_PALETTE().size()));
        QCOMPARE(Constants::GH_COMMITS_HISTORY, Constants::GH_REPO + QStringLiteral("/commits"));
        QVERIFY(!Constants::APP_PATH().isEmpty());
    }
};

QTEST_MAIN(TestApplicationViewModel)
#include "test_application_viewmodel.moc"
