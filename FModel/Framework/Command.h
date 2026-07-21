#pragma once
// Ported from FModel/Framework/Command.cs — the ICommand base used to bind UI controls to actions.
//
// C#'s ICommand exposes Execute(object)/CanExecute(object) and a CanExecuteChanged event; the abstract
// `Command` adds RaiseCanExecuteChanged(). In Qt the event becomes a signal, and `object parameter`
// becomes a QVariant (its natural boxed-value analog).

#include <QObject>
#include <QVariant>

namespace FModel::Framework
{
    class Command : public QObject
    {
        Q_OBJECT

    public:
        explicit Command(QObject* parent = nullptr) : QObject(parent) {}

        virtual void execute(const QVariant& parameter) = 0;
        virtual bool canExecute(const QVariant& parameter) = 0;

        void raiseCanExecuteChanged() { emit canExecuteChanged(); }

    signals:
        void canExecuteChanged();
    };
}
