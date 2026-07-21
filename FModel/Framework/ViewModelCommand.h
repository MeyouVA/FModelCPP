#pragma once
// Ported from FModel/Framework/ViewModelCommand.cs — a Command bound to a context view-model.
//
// C# holds the context view-model through a WeakReference so a command never keeps its view-model
// alive. Qt's QPointer<T> has exactly this semantics for QObjects: it becomes null when the pointed-to
// object is destroyed. Since ViewModel derives from QObject, QPointer models the WeakReference directly.
//
// This is a class template (C#'s `ViewModelCommand<TContextViewModel>`). It declares no signals or slots
// of its own — it only overrides Command's virtuals and inherits its canExecuteChanged signal — so it
// needs no Q_OBJECT / moc pass despite being a template.

#include <QPointer>

#include "Command.h"

namespace FModel::Framework
{
    template <typename TContextViewModel>
    class ViewModelCommand : public Command
    {
    public:
        explicit ViewModelCommand(TContextViewModel* contextViewModel, QObject* parent = nullptr)
            : Command(parent), _parent(contextViewModel) {}

        // Returns the live context view-model, or null once it has been destroyed.
        TContextViewModel* contextViewModel() const
        {
            return _parent.isNull() ? nullptr : _parent.data();
        }

        // sealed override: forwards to the strongly-typed overloads below.
        void execute(const QVariant& parameter) final
        {
            execute(contextViewModel(), parameter);
        }

        bool canExecute(const QVariant& parameter) final
        {
            return canExecute(contextViewModel(), parameter);
        }

        virtual void execute(TContextViewModel* contextViewModel, const QVariant& parameter) = 0;

        virtual bool canExecute(TContextViewModel* /*contextViewModel*/, const QVariant& /*parameter*/)
        {
            return true;
        }

    private:
        QPointer<TContextViewModel> _parent;
    };
}
