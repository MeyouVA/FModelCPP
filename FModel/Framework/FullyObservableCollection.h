#pragma once
// Ported from FModel/Framework/FullyObservableCollection.cs — an observable collection that also forwards
// its ITEMS' property changes, tagged with the item's index.
//
// The AES manager is the only consumer and it needs exactly that: the user edits a key in one row, and the
// view-model has to know *which* row changed to write the key back to the right settings entry.
//
// Deliberate differences from C#:
//   * The generic constraint is `where T : INotifyPropertyChanged`; here T is a pointer to a ViewModel, and
//     the subscription is a Qt connection to its propertyChanged signal.
//   * C# subscribes and unsubscribes inside OnCollectionChanged by inspecting the event's Old/New items.
//     There is no such event payload here (see RangeObservableCollection.h — the signal carries only the
//     action), so add()/clear() do the bookkeeping directly. Same effect, fewer moving parts.
//   * ItemPropertyChangedEventArgs (index + property name) becomes the two arguments of the
//     itemPropertyChanged signal.
//   * C# throws ArgumentException when a notification arrives from an item that is not in the collection.
//     That cannot happen here: the connection is made when the item is added and dropped when it is
//     removed, so the index lookup is always valid.

#include <QList>
#include <QObject>
#include <QString>

#include "ViewModel.h"

namespace FModel::Framework
{
    // The moc-visible half, for the same reason RangeObservableCollection has one: moc does not instantiate
    // templates.
    class FullyObservableCollectionBase : public QObject
    {
        Q_OBJECT

    public:
        explicit FullyObservableCollectionBase(QObject* parent = nullptr) : QObject(parent) {}

    signals:
        // C#'s ItemPropertyChanged, carrying ItemPropertyChangedEventArgs' two members.
        void itemPropertyChanged(int collectionIndex, const QString& propertyName);
    };

    template <typename T>
    class FullyObservableCollection : public FullyObservableCollectionBase
    {
    public:
        explicit FullyObservableCollection(QObject* parent = nullptr)
            : FullyObservableCollectionBase(parent) {}

        // C#'s FullyObservableCollection(IEnumerable<T>) — fills, then ObserveAll().
        FullyObservableCollection(const QList<T>& items, QObject* parent = nullptr)
            : FullyObservableCollectionBase(parent)
        {
            for (T item : items)
                add(item);
        }

        const QList<T>& items() const { return _items; }
        qsizetype count() const { return _items.size(); }
        T operator[](qsizetype index) const { return _items[index]; }

        auto begin() const { return _items.begin(); }
        auto end() const { return _items.end(); }

        void add(T item)
        {
            _items.append(item);
            QObject::connect(item, &ViewModel::propertyChanged, this,
                             [this, item](const QString& propertyName)
                             {
                                 const qsizetype index = _items.indexOf(item);
                                 if (index >= 0)
                                     emit itemPropertyChanged(static_cast<int>(index), propertyName);
                             });
        }

        void clear()
        {
            for (T item : _items)
                QObject::disconnect(item, &ViewModel::propertyChanged, this, nullptr);
            _items.clear();
        }

    private:
        QList<T> _items;
    };
}
