#pragma once
// Ported from FModel/ViewModels/ThreadWorkerViewModel.cs — the single funnel every long operation goes
// through: loading archives, mounting, populating the tree, extracting.
//
// C# runs each job on the thread pool (`await Task.Run(() => job(token))`) and drives the status bar around
// it — Loading while the queue drains, then Completed, or Stopped on cancellation, or Failed on an
// exception. The port keeps ALL of that except the thread: `begin()` runs the job synchronously on the
// caller's thread. That is a real behavioural difference (the UI does not stay responsive during a load),
// and it is deliberate for now — a threading layer would have to arrive with cancellation, progress
// marshalling and the dispatcher hops the view-models still take for granted. Everything else — the queue,
// the status transitions, the "already busy" signal, the cancellation flag — behaves as upstream.
//
// Deliberate differences from C#:
//   * `CancellationTokenSource` becomes FCancellationToken: a flag the job polls through
//     throwIfCancellationRequested(). With synchronous execution nothing can call cancel() *during* a job
//     from the same thread, so cancel() takes effect on the next queued job. `CanBeCanceled` keeps its
//     meaning (a token exists == an operation is in flight).
//   * The two "signal" properties (`StatusChangeAttempted`, `OperationCancelled`) are set true-then-false in
//     C# purely to fire a property change that a UI trigger listens for. That is preserved verbatim; the
//     stored value is always false afterwards.
//   * The `IsSnooperOpen` guard at the top of Begin and inside the cancellation arm needs the 3D viewer,
//     which is unported: those two branches are skipped and the rest of the guard is intact.
//   * The three FLogger arms in the exception handler (MappingException, VersionException, everything else)
//     are kept as a classified reason on the `jobFailed` signal, since FLogger is unported.

#include <exception>
#include <functional>

#include <QObject>
#include <QString>

#include "../Framework/AsyncQueue.h"
#include "../Framework/ViewModel.h"

namespace FModel::ViewModels
{
    class ApplicationViewModel;

    // C#'s CancellationToken as far as this port uses it: cooperative, polled by the job.
    class FCancellationToken
    {
    public:
        bool isCancellationRequested() const { return _cancelled; }
        void cancel() { _cancelled = true; }
        // C#'s ThrowIfCancellationRequested — the OperationCanceledException every job loop polls with.
        void throwIfCancellationRequested() const;

    private:
        bool _cancelled = false;
    };

    // C#'s OperationCanceledException.
    class FOperationCanceledException : public std::exception
    {
    public:
        const char* what() const noexcept override { return "The operation was canceled."; }
    };

    class ThreadWorkerViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit ThreadWorkerViewModel(QObject* parent = nullptr);

        // C# reads ApplicationService.ApplicationView; the locator is unported, so the worker is handed the
        // view-model whose Status it drives. Null is tolerated (the tests and a headless run).
        void setApplicationView(ApplicationViewModel* applicationView);
        ApplicationViewModel* applicationView() const { return _applicationView; }

        bool statusChangeAttempted() const { return _statusChangeAttempted; }
        bool operationCancelled() const { return _operationCancelled; }

        bool canBeCanceled() const { return _currentCancellation != nullptr; }

        // C#'s `Task Begin(Action<CancellationToken>)`.
        void begin(const std::function<void(FCancellationToken&)>& action);

        void cancel();

        // C#'s SignalOperationInProgress: raises the property twice so a UI trigger fires.
        void signalOperationInProgress();

    signals:
        // Where C# hands the exception to FLogger. `reason` classifies it the way that switch does:
        // "MappingException", "VersionException" or "Exception".
        void jobFailed(const QString& reason, const QString& message);

    private:
        void processQueues();
        void setCurrentCancellation(FCancellationToken* value);
        void setStatusChangeAttempted(bool value);
        void setOperationCancelled(bool value);

        ApplicationViewModel* _applicationView = nullptr;
        bool _statusChangeAttempted = false;
        bool _operationCancelled = false;
        FCancellationToken* _currentCancellation = nullptr;
        bool _draining = false; // stands in for C#'s SemaphoreSlim(1) — see AsyncQueue.h
        Framework::AsyncQueue<std::function<void(FCancellationToken&)>> _jobs;
    };
}
