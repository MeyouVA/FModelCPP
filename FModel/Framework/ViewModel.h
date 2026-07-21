#pragma once
// Ported from FModel/Framework/ViewModel.cs — the MVVM base class every view-model derives from.
//
// The C# type is `ViewModel : INotifyPropertyChanged, INotifyDataErrorInfo, IDataErrorInfo`.
// In Qt the change-notification role of INotifyPropertyChanged is played by a QObject signal
// (propertyChanged), so ViewModel derives from QObject. The per-property validation-error
// dictionary and the IDataErrorInfo indexer / Error property are kept faithfully.
//
// C# has [CallerMemberName], which fills in the property name at the call site; C++ has no
// equivalent, so SetProperty takes the property name explicitly.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>

namespace FModel::Framework
{
    class ViewModel : public QObject
    {
        Q_OBJECT

    public:
        explicit ViewModel(QObject* parent = nullptr) : QObject(parent) {}

        // IDataErrorInfo indexer: this[propertyName]. An empty name returns Error.
        QString operator[](const QString& propertyName) const;

        // IDataErrorInfo.Error — all current errors joined together.
        QString error() const;

        // INotifyDataErrorInfo.HasErrors
        bool hasErrors() const { return !_validationErrors.isEmpty(); }

        // INotifyDataErrorInfo.GetErrors
        QStringList getErrors(const QString& propertyName) const;

        void addValidationError(const QString& propertyName, const QString& errorMessage);
        void clearValidationErrors(const QString& propertyName);

    signals:
        // INotifyPropertyChanged.PropertyChanged
        void propertyChanged(const QString& propertyName);
        // INotifyDataErrorInfo.ErrorsChanged (declared for parity; not raised by the C# source either)
        void errorsChanged(const QString& propertyName);

    protected:
        void raisePropertyChanged(const QString& propertyName) { emit propertyChanged(propertyName); }

        // SetProperty<T>(ref storage, value, propertyName): assigns and raises PropertyChanged only
        // when the value actually changes, returning whether a change occurred.
        template <typename T>
        bool setProperty(T& storage, const T& value, const QString& propertyName)
        {
            if (storage == value)
                return false;

            storage = value;
            raisePropertyChanged(propertyName);
            return true;
        }

    private:
        QStringList getAllErrors() const;

        QHash<QString, QStringList> _validationErrors;
    };
}
