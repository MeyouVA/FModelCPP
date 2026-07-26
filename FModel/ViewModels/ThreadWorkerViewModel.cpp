// Ported from FModel/ViewModels/ThreadWorkerViewModel.cs
#include "ThreadWorkerViewModel.h"

#include "UE4/Exceptions/ParserException.h"

#include "ApplicationViewModel.h"
#include "../Framework/FStatus.h"

namespace FModel::ViewModels
{
    void FCancellationToken::throwIfCancellationRequested() const
    {
        if (_cancelled)
            throw FOperationCanceledException();
    }

    ThreadWorkerViewModel::ThreadWorkerViewModel(QObject* parent)
        : ViewModel(parent)
    {
    }

    void ThreadWorkerViewModel::setApplicationView(ApplicationViewModel* applicationView)
    {
        _applicationView = applicationView;
    }

    void ThreadWorkerViewModel::setStatusChangeAttempted(bool value)
    {
        setProperty(_statusChangeAttempted, value, QStringLiteral("StatusChangeAttempted"));
    }

    void ThreadWorkerViewModel::setOperationCancelled(bool value)
    {
        setProperty(_operationCancelled, value, QStringLiteral("OperationCancelled"));
    }

    void ThreadWorkerViewModel::setCurrentCancellation(FCancellationToken* value)
    {
        if (_currentCancellation == value)
            return;

        delete _currentCancellation;
        _currentCancellation = value;
        raisePropertyChanged(QStringLiteral("CurrentCancellationTokenSource"));
        raisePropertyChanged(QStringLiteral("CanBeCanceled"));
    }

    void ThreadWorkerViewModel::begin(const std::function<void(FCancellationToken&)>& action)
    {
        // C# first closes the Snooper if it is open (unported), and otherwise refuses to start while the app
        // is not ready — which is how a second load is rejected while one is running.
        if (_applicationView != nullptr && !_applicationView->status()->isReady())
        {
            signalOperationInProgress();
            return;
        }

        if (_currentCancellation == nullptr) // C#'s `??=`
            setCurrentCancellation(new FCancellationToken());

        _jobs.enqueue(action);
        processQueues();
    }

    void ThreadWorkerViewModel::cancel()
    {
        if (!canBeCanceled())
        {
            signalOperationInProgress();
            return;
        }

        _currentCancellation->cancel();
    }

    void ThreadWorkerViewModel::processQueues()
    {
        if (_jobs.count() <= 0)
            return;

        // C#'s SemaphoreSlim(1): a job that enqueues another job must not start a second drain loop — the
        // outer one picks it up, because it re-checks the queue after every item.
        if (_draining)
            return;

        Framework::FStatus* status = _applicationView != nullptr ? _applicationView->status() : nullptr;
        if (status != nullptr)
            status->setStatus(EStatusKind::Loading);

        _draining = true;
        while (auto job = _jobs.tryDequeue())
        {
            try
            {
                (*job)(*_currentCancellation);
            }
            catch (const FOperationCanceledException&)
            {
                if (status != nullptr)
                    status->setStatus(EStatusKind::Stopped);
                setCurrentCancellation(nullptr); // kill token
                setOperationCancelled(true);
                setOperationCancelled(false);
                _draining = false;
                return;
            }
            catch (const std::exception& e)
            {
                if (status != nullptr)
                    status->setStatus(EStatusKind::Failed);
                setCurrentCancellation(nullptr); // kill token

                // C# switches on the exception type to pick the FLogger message. ParserException is the
                // closest ported analogue of both MappingException and VersionException — neither is a
                // separate type here — so the classification is by message, and the caller decides how to
                // present it.
                const QString message = QString::fromUtf8(e.what());
                QString reason = QStringLiteral("Exception");
                if (message.contains(QStringLiteral("mapping"), Qt::CaseInsensitive) ||
                    message.contains(QStringLiteral("unversioned"), Qt::CaseInsensitive))
                    reason = QStringLiteral("MappingException");
                else if (message.contains(QStringLiteral("version"), Qt::CaseInsensitive))
                    reason = QStringLiteral("VersionException");

                emit jobFailed(reason, message);
                _draining = false;
                return;
            }
        }
        _draining = false;

        if (status != nullptr)
            status->setStatus(EStatusKind::Completed);
        setCurrentCancellation(nullptr); // kill token
    }

    void ThreadWorkerViewModel::signalOperationInProgress()
    {
        setStatusChangeAttempted(true);
        setStatusChangeAttempted(false);
    }
}
