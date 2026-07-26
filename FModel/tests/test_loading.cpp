// Behavioural tests for the game-loading pipeline: ThreadWorkerViewModel (the job funnel every step runs
// through), GameDirectoryViewModel (the Archives tab), AesManagerViewModel (the key rows and what an edit
// writes back), Helper::fixKey, and LoadCommand's filters.
//
// None of this touches a real game: the provider needs a directory to exist, so the tests that need one
// build a throwaway tree on disk. The pieces that decide *what the user sees* — which archives are listed,
// which files become tree nodes, which key edit lands in which settings entry — are all reachable without
// one, and those are the ones a port gets wrong quietly.

#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "Compression/CompressionMethod.h"
#include "FileProvider/Objects/GameFile.h"

#include "Constants.h"
#include "Enums.h"
#include "Framework/AsyncQueue.h"
#include "Framework/FStatus.h"
#include "Framework/FullyObservableCollection.h"
#include "Helper.h"
#include "Settings/DirectorySettings.h"
#include "Settings/UserSettings.h"
#include "ViewModels/AesManagerViewModel.h"
#include "ViewModels/ApplicationViewModel.h"
#include "ViewModels/CUE4ParseViewModel.h"
#include "ViewModels/GameDirectoryViewModel.h"
#include "ViewModels/GameSelectorViewModel.h"
#include "ViewModels/LoadingModesViewModel.h"
#include "ViewModels/ThreadWorkerViewModel.h"
#include "ViewModels/Commands/LoadCommand.h"

using namespace FModel;
using namespace FModel::ViewModels;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

class TestLoading : public QObject
{
    Q_OBJECT

    static Settings::UserSettings* freshSettings()
    {
        auto* settings = new Settings::UserSettings();
        auto* dir = Settings::DirectorySettings::Default(QStringLiteral("TestGame"), QStringLiteral("C:/Game/Paks"));
        dir->setParent(settings);
        settings->setCurrentDir(dir);
        Settings::UserSettings::SetDefault(settings);
        return settings;
    }

private slots:
    void init() { freshSettings(); }

    // --- Framework/AsyncQueue -------------------------------------------------------------------------

    void asyncQueueIsFifo()
    {
        Framework::AsyncQueue<int> queue;
        QCOMPARE(queue.count(), 0);
        QVERIFY(!queue.tryDequeue().has_value());

        queue.enqueue(1);
        queue.enqueue(2);
        QCOMPARE(queue.count(), 2);
        QCOMPARE(queue.tryDequeue().value(), 1);
        QCOMPARE(queue.tryDequeue().value(), 2);
        QVERIFY(!queue.tryDequeue().has_value());
    }

    // --- ThreadWorkerViewModel ------------------------------------------------------------------------

    void workerRunsJobsAndDrivesTheStatus()
    {
        ApplicationViewModel vm;
        ThreadWorkerViewModel* worker = vm.threadWorker();
        QVERIFY(!worker->canBeCanceled());

        int ran = 0;
        worker->begin([&ran](FCancellationToken&) { ++ran; });
        QCOMPARE(ran, 1);
        // The queue drained, so the token is dropped and the status lands on Completed.
        QVERIFY(!worker->canBeCanceled());
        QCOMPARE(vm.status()->kind(), EStatusKind::Completed);
    }

    void aJobThatStartsAnotherIsRefusedNotQueued()
    {
        ApplicationViewModel vm;
        ThreadWorkerViewModel* worker = vm.threadWorker();

        QStringList order;
        worker->begin([&](FCancellationToken&)
        {
            order.append(QStringLiteral("outer"));
            // The status is Loading for the whole drain, so Begin's `!Status.IsReady` guard rejects this
            // outright — it is never queued. That is C#'s behaviour too, and it is the mechanism that stops
            // a second Load while one is running.
            worker->begin([&order](FCancellationToken&) { order.append(QStringLiteral("inner")); });
            order.append(QStringLiteral("outer-end"));
        });

        QCOMPARE(order, QStringList({QStringLiteral("outer"), QStringLiteral("outer-end")}));
    }

    void cancellationStopsTheQueueAndSignals()
    {
        ApplicationViewModel vm;
        ThreadWorkerViewModel* worker = vm.threadWorker();
        QSignalSpy changed(worker, &Framework::ViewModel::propertyChanged);

        worker->begin([](FCancellationToken& token)
        {
            token.cancel();
            token.throwIfCancellationRequested();
        });

        QCOMPARE(vm.status()->kind(), EStatusKind::Stopped);
        QVERIFY(!worker->canBeCanceled());
        // OperationCancelled is set true then false purely to fire the notification.
        QVERIFY(!worker->operationCancelled());
        bool sawCancelled = false;
        for (const QList<QVariant>& args : changed)
            sawCancelled = sawCancelled || args[0].toString() == QStringLiteral("OperationCancelled");
        QVERIFY(sawCancelled);
    }

    void aFailedJobClassifiesTheException()
    {
        ApplicationViewModel vm;
        ThreadWorkerViewModel* worker = vm.threadWorker();
        QSignalSpy failed(worker, &ThreadWorkerViewModel::jobFailed);

        worker->begin([](FCancellationToken&)
        {
            throw std::runtime_error("Package has unversioned properties but mapping file is missing");
        });

        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed[0][0].toString(), QStringLiteral("MappingException"));
        QCOMPARE(vm.status()->kind(), EStatusKind::Failed);
    }

    void beginIsRefusedWhileTheAppIsBusy()
    {
        ApplicationViewModel vm;
        ThreadWorkerViewModel* worker = vm.threadWorker();
        vm.status()->setStatus(EStatusKind::Loading); // IsReady == false

        bool ran = false;
        QSignalSpy changed(worker, &Framework::ViewModel::propertyChanged);
        worker->begin([&ran](FCancellationToken&) { ran = true; });

        QVERIFY(!ran);
        bool sawAttempt = false;
        for (const QList<QVariant>& args : changed)
            sawAttempt = sawAttempt || args[0].toString() == QStringLiteral("StatusChangeAttempted");
        QVERIFY(sawAttempt);
    }

    // --- Helper::fixKey -------------------------------------------------------------------------------

    void fixKeyNormalisesAndKeepsItsQuirks()
    {
        const QString body = QString(64, QLatin1Char('a'));
        QCOMPARE(Helper::fixKey(QString()), QString());
        // No prefix -> one is added; the body is uppercased; the 'x' stays lowercase.
        QCOMPARE(Helper::fixKey(body), QStringLiteral("0x") + QString(64, QLatin1Char('A')));
        // An existing prefix is replaced, not doubled — including an uppercase "0X".
        QCOMPARE(Helper::fixKey(QStringLiteral("0X") + body), QStringLiteral("0x") + QString(64, QLatin1Char('A')));
        QCOMPARE(Helper::fixKey(QStringLiteral("  0x") + body + QStringLiteral("  ")),
                 QStringLiteral("0x") + QString(64, QLatin1Char('A')));

        // The length guard is `> sizeof(char) * (2 + 32)` and sizeof(char) is 2 in C#, so the limit is 68,
        // not the 66 a real key occupies: a 68-character key passes and comes back OVER-long.
        QCOMPARE(Helper::fixKey(QString(69, QLatin1Char('a'))), QString());
        QCOMPARE(Helper::fixKey(QString(68, QLatin1Char('a'))).size(), 70);
    }

    // --- GameDirectoryViewModel -----------------------------------------------------------------------

    void archiveListHidesTheContainersItShould()
    {
        // The regex is `^(?!global|pakchunk.+(optional|ondemand)\-).+(pak|utoc)$`, case-insensitive.
        QVERIFY(GameDirectoryViewModel::isVisibleArchive(QStringLiteral("pakchunk0-WindowsClient.pak")));
        QVERIFY(GameDirectoryViewModel::isVisibleArchive(QStringLiteral("pakchunk0-WindowsClient.utoc")));
        QVERIFY(GameDirectoryViewModel::isVisibleArchive(QStringLiteral("FactoryGame-Windows.pak")));

        QVERIFY(!GameDirectoryViewModel::isVisibleArchive(QStringLiteral("global.utoc")));
        QVERIFY(!GameDirectoryViewModel::isVisibleArchive(QStringLiteral("pakchunk10optional-WindowsClient.utoc")));
        QVERIFY(!GameDirectoryViewModel::isVisibleArchive(QStringLiteral("pakchunk10ondemand-WindowsClient.utoc")));
        QVERIFY(!GameDirectoryViewModel::isVisibleArchive(QStringLiteral("something.sig")));
    }

    void looseFilesRowIsCreatedOnceAndAccumulates()
    {
        GameDirectoryViewModel directory;
        directory.addLooseFiles(0); // below 1 is ignored outright
        QCOMPARE(directory.directoryFiles().count(), 0);

        directory.addLooseFiles(3);
        QCOMPARE(directory.directoryFiles().count(), 1);
        directory.addLooseFiles(4);
        QCOMPARE(directory.directoryFiles().count(), 1); // same row...
        QCOMPARE(directory.directoryFiles()[0]->fileCount(), 7); // ...with both counts
        QVERIFY(directory.directoryFiles()[0]->isLooseFilesContainer());
        QVERIFY(directory.directoryFiles()[0]->isEnabled());
    }

    void archiveViewSortsContainersBeforeLooseFiles()
    {
        GameDirectoryViewModel directory;
        directory.directoryFiles().add(new FileItem(QStringLiteral("zulu.pak"), 0, 0, false, &directory));
        directory.addLooseFiles(1);
        directory.directoryFiles().add(new FileItem(QStringLiteral("alpha.pak"), 0, 0, false, &directory));

        const QList<FileItem*> view = directory.directoryFilesView().items();
        QCOMPARE(view.size(), 3);
        QCOMPARE(view[0]->name(), QStringLiteral("alpha.pak"));
        QCOMPARE(view[1]->name(), QStringLiteral("zulu.pak"));
        QCOMPARE(view[2]->name(), QStringLiteral("Loose Files")); // IsLooseFilesContainer sorts last
    }

    // --- AesManagerViewModel --------------------------------------------------------------------------

    void aesRowsSkipTheZeroGuidAndDeduplicate()
    {
        ApplicationViewModel vm;
        GameDirectoryViewModel* directory = vm.cue4Parse()->gameDirectory();

        FGuid guidA(1u, 2u, 3u, 4u);
        auto* encryptedA = new FileItem(QStringLiteral("a.pak"), 0, 0, false, directory);
        encryptedA->setGuid(guidA);
        auto* encryptedB = new FileItem(QStringLiteral("b.pak"), 0, 0, false, directory);
        encryptedB->setGuid(guidA); // same GUID: one row, not two
        auto* plain = new FileItem(QStringLiteral("c.pak"), 0, 0, false, directory);
        plain->setGuid(Constants::ZERO_GUID); // unencrypted: no row at all
        directory->directoryFiles().add(encryptedA);
        directory->directoryFiles().add(encryptedB);
        directory->directoryFiles().add(plain);

        vm.aesManager()->initAes();
        // The main static key, plus exactly one dynamic row.
        QCOMPARE(vm.aesManager()->aesKeys()->count(), 2);
        QCOMPARE((*vm.aesManager()->aesKeys())[0]->name(), QStringLiteral("Main Static Key"));
        QCOMPARE((*vm.aesManager()->aesKeys())[1], encryptedA);
    }

    void editingAKeyWritesBackAndFlagsAChange()
    {
        ApplicationViewModel vm;
        vm.aesManager()->initAes();
        QVERIFY(!vm.aesManager()->hasChange());

        const QString key = QStringLiteral("0x") + QString(64, QLatin1Char('B'));
        (*vm.aesManager()->aesKeys())[0]->setKey(key);
        QVERIFY(vm.aesManager()->hasChange());

        vm.aesManager()->setAesKeys();
        QCOMPARE(Settings::UserSettings::Default()->currentDir()->aesKeys().MainKey, key);

        // Re-assigning the same key raises no NEW change, but HasChange latches (nothing clears it).
        (*vm.aesManager()->aesKeys())[0]->setKey(key);
        QVERIFY(vm.aesManager()->hasChange());
    }

    // --- LoadCommand ----------------------------------------------------------------------------------

    void packageCountLabelGroupsDigits()
    {
        QCOMPARE(Commands::LoadCommand::packageCountLabel(0), QStringLiteral("0 Packages"));
        QCOMPARE(Commands::LoadCommand::packageCountLabel(999), QStringLiteral("999 Packages"));
        QCOMPARE(Commands::LoadCommand::packageCountLabel(1000), QStringLiteral("1 000 Packages"));
        QCOMPARE(Commands::LoadCommand::packageCountLabel(48616), QStringLiteral("48 616 Packages"));
        QCOMPARE(Commands::LoadCommand::packageCountLabel(1234567), QStringLiteral("1 234 567 Packages"));
    }

    void loadIsRefusedWithNoFiles()
    {
        ApplicationViewModel vm;
        auto* command = vm.loadingModes()->loadCommand();
        QSignalSpy refused(command, &Commands::LoadCommand::refused);

        // The provider exists (the directory does not, so it found nothing) and holds no files.
        command->execute(QVariant(QVariantList{}));
        QCOMPARE(refused.count(), 1);
        QVERIFY(refused[0][0].toString().contains(QStringLiteral("No files were found")));
    }

    // --- CUE4ParseViewModel + GameSelectorViewModel ----------------------------------------------------

    void liveServiceEntriesAreRejectedRatherThanMisread()
    {
        auto* settings = Settings::UserSettings::Default();
        settings->currentDir()->setGameDirectory(Constants::_FN_LIVE_TRIGGER);

        ApplicationViewModel vm;
        QVERIFY(vm.cue4Parse()->isUnsupportedLiveService());
        QCOMPARE(vm.cue4Parse()->provider(), nullptr);
        // Everything downstream has to tolerate a null provider rather than crash.
        vm.cue4Parse()->initialize();
        vm.cue4Parse()->clearProvider();
        QCOMPARE(vm.cue4Parse()->loadVirtualPaths(), 0);
        QCOMPARE(vm.cue4Parse()->verifyConsoleVariables().size(), 0);
        QCOMPARE(vm.gameDisplayName(), QStringLiteral("Unknown"));
    }

    void selectorListsTheLiveEntriesAndAddsAManualOne()
    {
        // Braces, not parentheses: `selector(QString())` is a function declaration (most vexing parse).
        GameSelectorViewModel selector{QString()};
        // Only the two probe-free entries survive; the launcher scanners are unported.
        QCOMPARE(selector.detectedDirectories().size(), 2);
        QVERIFY(selector.selectedDirectory() != nullptr);
        QCOMPARE(selector.selectedDirectory()->gameDirectory(), Constants::_FN_LIVE_TRIGGER);

        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString paks = QDir::toNativeSeparators(tmp.path());
        selector.addUndetectedDir(paks);

        QCOMPARE(selector.detectedDirectories().size(), 3);
        QCOMPARE(selector.selectedDirectory()->gameDirectory(), paks);
        QVERIFY(selector.selectedDirectory()->isManual());
        // Adding also files it into settings, which is what makes it reappear next launch.
        QVERIFY(Settings::UserSettings::Default()->perDirectory(paks) != nullptr);

        selector.deleteSelectedGame();
        QCOMPARE(selector.detectedDirectories().size(), 2);
        QCOMPARE(Settings::UserSettings::Default()->perDirectory(paks), nullptr);
    }

    void selectorPreselectsTheConfiguredDirectory()
    {
        // A directory that is already in the list is selected rather than re-added.
        GameSelectorViewModel selector(Constants::_VAL_LIVE_TRIGGER);
        QCOMPARE(selector.detectedDirectories().size(), 2);
        QCOMPARE(selector.selectedDirectory()->gameDirectory(), Constants::_VAL_LIVE_TRIGGER);
    }

    void ueGameListMatchesTheSettingsDialogs()
    {
        // C# declares EnumerateUeGames twice, identically; the port has one implementation behind both.
        const QList<CUE4Parse::UE4::Versions::EGame> games = GameSelectorViewModel::enumerateUeGames();
        QVERIFY(games.size() > 200);
        // Game-specific members (low byte set) come before the base engine versions.
        qsizetype firstBase = -1;
        for (qsizetype i = 0; i < games.size(); ++i)
        {
            if ((static_cast<int>(games[i]) & 0xFF) == 0) { firstBase = i; break; }
        }
        QVERIFY(firstBase > 0);
        for (qsizetype i = firstBase; i < games.size(); ++i)
            QVERIFY((static_cast<int>(games[i]) & 0xFF) == 0);
    }
};

QTEST_MAIN(TestLoading)
#include "test_loading.moc"
