#pragma once
// NEW port-support code — there is no CollectionView.cs. This stands in for WPF's
// System.ComponentModel.ICollectionView / System.Windows.Data.ListCollectionView, the same way
// CUE4Parse's Utils/Json.h stands in for Newtonsoft.Json.
//
// WPF wraps a collection in a *view*: a live, sorted, filtered projection that the UI binds to while the
// underlying collection stays in insertion order. Three view-models in this port need exactly that and
// nothing more — AssetsListViewModel.AssetsView, TreeItem.FoldersView and TreeItem.FilteredFoldersView —
// so this is deliberately the smallest thing that reproduces their behaviour:
//
//   * a sort comparison, standing in for `SortDescriptions = { new SortDescription(name, Ascending) }`.
//     C# names a property and WPF reflects on it; C++ has no reflection, so the caller passes the
//     comparison directly. std::stable_sort is used so equal keys keep insertion order.
//   * an optional Filter predicate, and Refresh() to re-run it.
//   * automatic invalidation when the source collection notifies. This is why RangeObservableCollection's
//     suppression flag matters: while notifications are muted the view does NOT re-materialise, which is
//     exactly the batching AssetsFolderViewModel.BulkPopulate relies on.
//
// Not modelled: grouping, currency (CurrentItem / MoveCurrentTo), live shaping, and the
// IEditableCollectionView editing transactions. No call site in the ported tree uses any of them.

#include <functional>
#include <algorithm>

#include <QList>
#include <QObject>

#include "RangeObservableCollection.h"

namespace FModel::Framework
{
    template <typename T>
    class CollectionView
    {
    public:
        using Filter = std::function<bool(const T&)>;
        using Less = std::function<bool(const T&, const T&)>;

        CollectionView(RangeObservableCollection<T>* source, Less sortBy)
            : _source(source), _sortBy(std::move(sortBy))
        {
            if (_source != nullptr)
            {
                // The source is the connection's context object, so the connection dies with it.
                QObject::connect(_source, &ObservableCollectionBase::collectionChanged, _source,
                                 [this](ObservableCollectionBase::NotifyCollectionChangedAction) { _dirty = true; });
            }
        }

        // ICollectionView.Filter
        void setFilter(Filter filter)
        {
            _filter = std::move(filter);
            _dirty = true;
        }

        bool hasFilter() const { return static_cast<bool>(_filter); }

        // ICollectionView.Refresh()
        void refresh() { _dirty = true; }

        const QList<T>& items() const
        {
            if (_dirty)
                materialise();
            return _view;
        }

        qsizetype count() const { return items().size(); }

        auto begin() const { return items().begin(); }
        auto end() const { return items().end(); }

    private:
        void materialise() const
        {
            _view.clear();
            if (_source != nullptr)
            {
                for (const T& item : _source->items())
                {
                    if (!_filter || _filter(item))
                        _view.append(item);
                }
                if (_sortBy)
                    std::stable_sort(_view.begin(), _view.end(), _sortBy);
            }
            _dirty = false;
        }

        RangeObservableCollection<T>* _source = nullptr;
        Less _sortBy;
        Filter _filter;
        mutable QList<T> _view;
        mutable bool _dirty = true;
    };
}
