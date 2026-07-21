// Behavioural tests for the ported MVVM framework layer (Framework/ViewModel, Command, ViewModelCommand).
#include <QtTest>
#include <QSignalSpy>

#include "Framework/ViewModel.h"
#include "Framework/Command.h"
#include "Framework/ViewModelCommand.h"
#include "Framework/FStatus.h"

using namespace FModel::Framework;
using FModel::EStatusKind;

// A concrete view-model with one observable property, mirroring how real view-models use SetProperty.
class SampleViewModel : public ViewModel
{
    Q_OBJECT
public:
    int count() const { return _count; }
    bool setCount(int value) { return setProperty(_count, value, QStringLiteral("count")); }

private:
    int _count = 0;
};

// A concrete ViewModelCommand that increments its context view-model's count.
class IncrementCommand : public ViewModelCommand<SampleViewModel>
{
public:
    using ViewModelCommand::ViewModelCommand;

    void execute(SampleViewModel* vm, const QVariant& parameter) override
    {
        if (vm)
            vm->setCount(vm->count() + parameter.toInt());
    }

    bool canExecute(SampleViewModel* vm, const QVariant&) override
    {
        return vm != nullptr && vm->count() < 3;
    }
};

class TestFramework : public QObject
{
    Q_OBJECT

private slots:
    // ViewModel.SetProperty raises PropertyChanged only when the value actually changes.
    void setProperty_raisesOnlyOnChange()
    {
        SampleViewModel vm;
        QSignalSpy spy(&vm, &ViewModel::propertyChanged);

        QVERIFY(vm.setCount(5));          // 0 -> 5: changed
        QCOMPARE(vm.count(), 5);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("count"));

        QVERIFY(!vm.setCount(5));         // 5 -> 5: no change, no signal
        QCOMPARE(spy.count(), 0);
    }

    // The IDataErrorInfo indexer / Error / HasErrors surface added errors and clear them.
    void validationErrors_roundTrip()
    {
        SampleViewModel vm;
        QVERIFY(!vm.hasErrors());
        QVERIFY(vm[QStringLiteral("count")].isEmpty());

        vm.addValidationError(QStringLiteral("count"), QStringLiteral("too small"));
        vm.addValidationError(QStringLiteral("count"), QStringLiteral("also odd"));
        QVERIFY(vm.hasErrors());
        QCOMPARE(vm.getErrors(QStringLiteral("count")).count(), 2);
        QVERIFY(vm[QStringLiteral("count")].contains(QStringLiteral("too small")));
        QVERIFY(vm.error().contains(QStringLiteral("also odd")));
        // Empty property name routes to Error.
        QCOMPARE(vm[QString()], vm.error());

        vm.clearValidationErrors(QStringLiteral("count"));
        QVERIFY(!vm.hasErrors());
        QVERIFY(vm[QStringLiteral("count")].isEmpty());
    }

    // ViewModelCommand forwards Execute/CanExecute to the context view-model. Commands are used through
    // the Command (ICommand) base in the UI, so the single-argument overloads are exercised via it.
    void command_executesAgainstContext()
    {
        SampleViewModel vm;
        IncrementCommand cmd(&vm);
        Command& c = cmd;

        QVERIFY(c.canExecute(QVariant()));   // count 0 < 3
        c.execute(2);
        QCOMPARE(vm.count(), 2);
        QVERIFY(c.canExecute(QVariant()));   // count 2 < 3
        c.execute(5);
        QCOMPARE(vm.count(), 7);
        QVERIFY(!c.canExecute(QVariant()));  // count 7 !< 3
    }

    // RaiseCanExecuteChanged fires the CanExecuteChanged signal.
    void command_raiseCanExecuteChanged()
    {
        SampleViewModel vm;
        IncrementCommand cmd(&vm);
        QSignalSpy spy(&cmd, &Command::canExecuteChanged);
        cmd.raiseCanExecuteChanged();
        QCOMPARE(spy.count(), 1);
    }

    // The QPointer weak reference becomes null when the context view-model is destroyed,
    // matching C#'s WeakReference: the command survives and CanExecute returns its default.
    void command_weakReferenceReleases()
    {
        auto* vm = new SampleViewModel();
        IncrementCommand cmd(vm);
        QCOMPARE(cmd.contextViewModel(), vm);

        delete vm;
        QCOMPARE(cmd.contextViewModel(), nullptr);
        Command& c = cmd;
        QVERIFY(!c.canExecute(QVariant())); // null context -> false, and no crash
    }

    // FStatus (a concrete ViewModel): a fresh instance is Loading and not ready, with label "Loading".
    void fstatus_initialState()
    {
        FStatus status;
        QCOMPARE(status.kind(), EStatusKind::Loading);
        QVERIFY(!status.isReady());
        QCOMPARE(status.label(), QStringLiteral("Loading"));
    }

    // SetStatus updates kind, label, and the derived IsReady, raising PropertyChanged for each.
    void fstatus_setStatus()
    {
        FStatus status;
        QSignalSpy spy(&status, &ViewModel::propertyChanged);

        status.setStatus(EStatusKind::Loading, QStringLiteral("Extracting"));
        QVERIFY(!status.isReady());
        QCOMPARE(status.label(), QStringLiteral("Loading Extracting"));

        status.setStatus(EStatusKind::Ready);
        QVERIFY(status.isReady());
        QCOMPARE(status.label(), QStringLiteral("Ready")); // non-Loading -> just the kind name

        status.setStatus(EStatusKind::Stopping);
        QVERIFY(!status.isReady()); // Stopping is not ready

        status.setStatus(EStatusKind::Completed);
        QVERIFY(status.isReady());
        QCOMPARE(status.label(), QStringLiteral("Completed"));

        // Change notifications were raised along the way (Kind / IsReady / Label).
        QVERIFY(spy.count() > 0);
    }

    // While Loading, UpdateStatusLabel uses the given prefix in place of the kind name.
    void fstatus_updateLabelPrefix()
    {
        FStatus status;
        status.setStatus(EStatusKind::Loading);
        status.updateStatusLabel(QStringLiteral("Pak"), QStringLiteral("Reading"));
        QCOMPARE(status.label(), QStringLiteral("Reading Pak"));
    }
};

QTEST_MAIN(TestFramework)
#include "test_framework.moc"
