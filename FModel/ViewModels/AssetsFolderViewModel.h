#pragma once
// Ported from FModel/ViewModels/AssetsFolderViewModel.cs — the explorer's folder tree: TreeItem (one node)
// and AssetsFolderViewModel (the roots, plus the bulk build that turns a flat file list into the tree).
//
// Deliberate differences from C#:
//   * TreeItem nodes are heap-allocated and parented (QObject ownership): every child node and every asset
//     row belongs to its parent node, and the roots belong to the AssetsFolderViewModel. C# leans on the GC.
//   * `Parent` is a plain pointer set at construction rather than an `init` property, and it is NOT an
//     owning link — the owner is the QObject parent, which is the same node.
//   * ICollectionView becomes Framework::CollectionView (see that header). `FoldersView` sorts by Header;
//     `FilteredFoldersView` sorts by Header AND filters; `CombinedEntries` is the CompositeCollection of
//     the filtered folders followed by the asset view. C# marshals CombinedEntries onto the WPF dispatcher
//     because it may be built from a worker thread; there is no such requirement here.
//   * CombinedEntries yields QObject* because its two halves have different element types (TreeItem* and
//     GameFileViewModel*) — C#'s CompositeCollection is likewise untyped. Consumers dynamic_cast, which is
//     exactly what ItemFilter does upstream.
//   * `OnSelectedCategoryChanged` is synchronous: C# awaits ResolveAsync(Category) over every asset before
//     refreshing, and nothing in the ported resolver blocks.
//   * BulkPopulate's dispatcher Invoke is dropped for the same reason; the two things it reaches for that
//     ARE unported — the provider's ProjectName and SearchVm.ChangeCollection — become a static seam and a
//     signal respectively. C#'s ProjectName lookup is itself a process-wide service-locator read.

#include <optional>

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "UE4/Versions/ObjectVersion.h"

#include "AssetsListViewModel.h"
#include "../Enums.h"
#include "../Framework/CollectionView.h"
#include "../Framework/RangeObservableCollection.h"
#include "../Framework/ViewModel.h"

namespace CUE4Parse::FileProvider::Objects { class GameFile; }

namespace FModel::ViewModels
{
    class TreeItem : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        // C#: TreeItem(string header, GameFile entry, string pathHere) with `Parent` set by the object
        // initialiser. The parent is folded into the constructor here because it is also the QObject owner.
        TreeItem(const QString& header, CUE4Parse::FileProvider::Objects::GameFile* entry,
                 const QString& pathHere, TreeItem* parent = nullptr);

        const QString& header() const { return _header; }

        bool isExpanded() const { return _isExpanded; }
        void setIsExpanded(bool value);

        bool isSelected() const { return _isSelected; }
        void setIsSelected(bool value);

        // Filled only when the entry came out of a mounted container (C#: `if (entry is VfsEntry vfsEntry)`).
        const QString& archive() const { return _archive; }
        const QString& mountPoint() const { return _mountPoint; }
        CUE4Parse::UE4::Versions::FPackageFileVersion version() const { return _version; }

        const QString& searchText() const { return _searchText; }
        void setSearchText(const QString& value);

        EAssetCategory selectedCategory() const { return _selectedCategory; }
        void setSelectedCategory(EAssetCategory value);

        const QString& pathAtThisPoint() const { return _pathAtThisPoint; }

        AssetsListViewModel* assetsList() const { return _assetsList; }

        Framework::RangeObservableCollection<TreeItem*>& folders() { return _folders; }
        const Framework::RangeObservableCollection<TreeItem*>& folders() const { return _folders; }

        Framework::CollectionView<TreeItem*>& foldersView() { return _foldersView; }
        Framework::CollectionView<TreeItem*>& filteredFoldersView() { return _filteredFoldersView; }

        // C#'s CompositeCollection: the filtered folders, then the assets.
        QList<QObject*> combinedEntries() const;

        TreeItem* parentItem() const { return _parentItem; }

        // C#'s ToString(): "<header> | N Folders | N Files".
        QString toString() const;

    private:
        void refreshFilters();
        // C#'s ItemFilter(object item, IEnumerable<string> filters) — the one predicate both views use.
        bool itemFilter(QObject* item, const QStringList& filters) const;
        // The `SearchText.Trim().Split(' ', RemoveEmptyEntries)` both call sites pass in.
        QStringList searchFilters() const;
        void onSelectedCategoryChanged();

        QString _header;
        bool _isExpanded = false;
        bool _isSelected = false;
        QString _archive;
        QString _mountPoint;
        CUE4Parse::UE4::Versions::FPackageFileVersion _version;
        QString _searchText;
        EAssetCategory _selectedCategory = EAssetCategory::All;
        QString _pathAtThisPoint;

        AssetsListViewModel* _assetsList = nullptr;
        Framework::RangeObservableCollection<TreeItem*> _folders;
        Framework::CollectionView<TreeItem*> _foldersView;
        Framework::CollectionView<TreeItem*> _filteredFoldersView;
        TreeItem* _parentItem = nullptr;
    };

    class AssetsFolderViewModel : public QObject
    {
        Q_OBJECT

    public:
        explicit AssetsFolderViewModel(QObject* parent = nullptr);

        // Stands in for ApplicationService.ApplicationView.CUE4Parse.Provider.ProjectName, which BulkPopulate
        // reads to decide which root to select. The provider is unported; empty means "no project name",
        // which lands on C#'s `?? treeItems[0]` fallback.
        static void setProjectName(const QString& projectName);
        static const QString& projectName();

        Framework::RangeObservableCollection<TreeItem*>& folders() { return _folders; }
        const Framework::RangeObservableCollection<TreeItem*>& folders() const { return _folders; }

        Framework::CollectionView<TreeItem*>& foldersView() { return _foldersView; }

        void bulkPopulate(const QList<CUE4Parse::FileProvider::Objects::GameFile*>& entries);

    signals:
        // Where C# calls ApplicationService.ApplicationView.CUE4Parse.SearchVm.ChangeCollection(entries).
        void deferred(const QString& what, const QString& waitingOn);

    private:
        // C#'s local static of the same name, hoisted to a member because C++ has no local functions.
        static void invokeOnCollectionChanged(TreeItem* item);

        Framework::RangeObservableCollection<TreeItem*> _folders;
        Framework::CollectionView<TreeItem*> _foldersView;
    };
}
