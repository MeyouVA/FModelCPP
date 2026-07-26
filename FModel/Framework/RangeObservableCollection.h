#pragma once
// Ported from FModel/Framework/RangeObservableCollection.cs — an ObservableCollection<T> that can be filled
// in bulk without raising one notification per item, and whose notifications can be muted outright.
//
// C# gets ObservableCollection<T> from the BCL: a list that raises CollectionChanged. Qt has no such type,
// so the list half is a QList<T> and the notification half is a signal. A class template cannot carry
// Q_OBJECT (moc does not instantiate templates), so the signal lives on a non-template QObject base and the
// template derives from it — the same split Qt itself uses for QAbstractItemModel's typed subclasses.
//
// Deliberate differences from C#:
//   * NotifyCollectionChangedEventArgs carries action + items + indices; only the *action* is ever read by
//     the port's callers (C# only ever constructs a bare Reset here), so the signal carries just that.
//   * AddRange throws ArgumentNullException on a null list; a QList reference cannot be null, so that check
//     has no counterpart. An empty list still ends with the Reset, as in C#.
//   * The collection is not itself an item container with indexer semantics — `items()` exposes the QList
//     directly. Every C# call site either iterates it, counts it, or appends to it.
//
// Note the C# ordering quirk, preserved: AddRange sets the suppression flag, adds, CLEARS the flag and then
// raises one Reset. So AddRange always ends unsuppressed — even if the caller had explicitly suppressed the
// collection with SetSuppressionState(true) beforehand. AssetsFolderViewModel.BulkPopulate depends on this:
// it suppresses the *tree items* it builds, then AddRange's clear is what re-arms the root collection.

#include <QList>
#include <QObject>

namespace FModel::Framework
{
    // The moc-visible half. C#'s System.Collections.Specialized.NotifyCollectionChangedAction lives here too,
    // since it is the signal's argument.
    class ObservableCollectionBase : public QObject
    {
        Q_OBJECT

    public:
        enum class NotifyCollectionChangedAction
        {
            Add,
            Remove,
            Replace,
            Move,
            Reset,
        };
        Q_ENUM(NotifyCollectionChangedAction)

        explicit ObservableCollectionBase(QObject* parent = nullptr) : QObject(parent) {}

    signals:
        void collectionChanged(NotifyCollectionChangedAction action);

    protected:
        // C#'s protected override OnCollectionChanged — the single place the suppression flag is honoured.
        void onCollectionChanged(NotifyCollectionChangedAction action)
        {
            if (!_suppressNotification)
                emit collectionChanged(action);
        }

        bool _suppressNotification = false;
    };

    template <typename T>
    class RangeObservableCollection : public ObservableCollectionBase
    {
    public:
        explicit RangeObservableCollection(QObject* parent = nullptr) : ObservableCollectionBase(parent) {}

        const QList<T>& items() const { return _items; }
        QList<T>& items() { return _items; }

        qsizetype count() const { return _items.size(); }
        bool isEmpty() const { return _items.isEmpty(); }
        const T& operator[](qsizetype index) const { return _items[index]; }
        T& operator[](qsizetype index) { return _items[index]; }

        auto begin() { return _items.begin(); }
        auto end() { return _items.end(); }
        auto begin() const { return _items.begin(); }
        auto end() const { return _items.end(); }

        // ObservableCollection<T>.Add
        void add(const T& item)
        {
            _items.append(item);
            onCollectionChanged(NotifyCollectionChangedAction::Add);
        }

        void clear()
        {
            _items.clear();
            onCollectionChanged(NotifyCollectionChangedAction::Reset);
        }

        void addRange(const QList<T>& list)
        {
            _suppressNotification = true;

            for (const T& item : list)
                add(item);

            _suppressNotification = false;
            onCollectionChanged(NotifyCollectionChangedAction::Reset);
        }

        void setSuppressionState(bool state) { _suppressNotification = state; }
        bool suppressionState() const { return _suppressNotification; }

        void invokeOnCollectionChanged(
            NotifyCollectionChangedAction changedAction = NotifyCollectionChangedAction::Reset)
        {
            onCollectionChanged(changedAction);
        }

    private:
        QList<T> _items;
    };
}
