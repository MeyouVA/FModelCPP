// Ported from FModel/ViewModels/AssetsFolderViewModel.cs
#include "AssetsFolderViewModel.h"

#include <algorithm>

#include <QStandardPaths>

#include "FileProvider/Objects/GameFile.h"
#include "UE4/VirtualFileSystem/VfsEntry.h"

#include "../Extensions/AssetCategoryExtensions.h"

namespace FModel::ViewModels
{
    using CUE4Parse::FileProvider::Objects::GameFile;
    using CUE4Parse::UE4::VirtualFileSystem::VfsEntry;
    using Extensions::AssetCategoryExtensions::isOfCategory;

    namespace
    {
        // SortDescription(nameof(Header), ListSortDirection.Ascending) — shared by all three views.
        bool byHeader(TreeItem* a, TreeItem* b) { return a->header() < b->header(); }

        // C#'s local static FindByHeaderOrNull: a linear scan, first match wins.
        TreeItem* findByHeaderOrNull(const Framework::RangeObservableCollection<TreeItem*>& list,
                                     const QString& header)
        {
            for (TreeItem* item : list.items())
            {
                if (item->header() == header)
                    return item;
            }
            return nullptr;
        }

        QString& projectNameSlot()
        {
            static QString projectName;
            return projectName;
        }
    }

    TreeItem::TreeItem(const QString& header, GameFile* entry, const QString& pathHere, TreeItem* parent)
        : ViewModel(parent),
          _header(header),
          _pathAtThisPoint(pathHere),
          _folders(this),
          _foldersView(&_folders, byHeader),
          _filteredFoldersView(&_folders, byHeader),
          _parentItem(parent)
    {
        // C# assigns Archive/MountPoint/Version through private SetProperty setters; nothing is listening
        // during construction, which is the only place they are written.
        if (auto* vfsEntry = dynamic_cast<VfsEntry*>(entry); vfsEntry != nullptr && vfsEntry->Vfs != nullptr)
        {
            _archive = QString::fromStdString(vfsEntry->Vfs->Name());
            _mountPoint = QString::fromStdString(vfsEntry->Vfs->MountPoint());
            _version = vfsEntry->Vfs->Ver();
        }

        _assetsList = new AssetsListViewModel(this);

        // Both views run the same predicate; the folder one is installed here rather than in the getter
        // because C# does the assets one in the constructor and builds the folder view lazily with its
        // Filter baked in. The observable behaviour is identical — the predicate reads SearchText live.
        _assetsList->assetsView().setFilter([this](GameFileViewModel* item)
        {
            return itemFilter(item, searchFilters());
        });
        _filteredFoldersView.setFilter([this](TreeItem* item)
        {
            return itemFilter(item, searchFilters());
        });
    }

    void TreeItem::setIsExpanded(bool value)
    {
        setProperty(_isExpanded, value, QStringLiteral("IsExpanded"));
    }

    void TreeItem::setIsSelected(bool value)
    {
        setProperty(_isSelected, value, QStringLiteral("IsSelected"));
    }

    void TreeItem::setSearchText(const QString& value)
    {
        if (setProperty(_searchText, value, QStringLiteral("SearchText")))
            refreshFilters();
    }

    void TreeItem::setSelectedCategory(EAssetCategory value)
    {
        if (setProperty(_selectedCategory, value, QStringLiteral("SelectedCategory")))
            onSelectedCategoryChanged();
    }

    QStringList TreeItem::searchFilters() const
    {
        return _searchText.trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    }

    void TreeItem::refreshFilters()
    {
        _assetsList->assetsView().refresh();
        _filteredFoldersView.refresh();
    }

    bool TreeItem::itemFilter(QObject* item, const QStringList& filters) const
    {
        if (auto* entry = qobject_cast<GameFileViewModel*>(item))
        {
            bool matchesSearch = filters.isEmpty();
            if (!matchesSearch)
            {
                const QString name = QString::fromStdString(entry->asset()->Name());
                matchesSearch = std::all_of(filters.begin(), filters.end(), [&name](const QString& x)
                {
                    return name.contains(x, Qt::CaseInsensitive);
                });
            }
            const bool matchesCategory = _selectedCategory == EAssetCategory::All ||
                                         isOfCategory(entry->assetCategory(), _selectedCategory);
            return matchesSearch && matchesCategory;
        }
        if (auto* folder = qobject_cast<TreeItem*>(item))
        {
            bool matchesSearch = filters.isEmpty();
            if (!matchesSearch)
            {
                const QString& folderHeader = folder->header();
                matchesSearch = std::all_of(filters.begin(), filters.end(), [&folderHeader](const QString& x)
                {
                    return folderHeader.contains(x, Qt::CaseInsensitive);
                });
            }
            // A folder is only ever shown on the "All" tab — folders carry no category of their own.
            const bool matchesCategory = _selectedCategory == EAssetCategory::All;
            return matchesSearch && matchesCategory;
        }
        return false;
    }

    void TreeItem::onSelectedCategoryChanged()
    {
        // C#: await Task.WhenAll(AssetsList.Assets.Select(a => a.ResolveAsync(EResolveCompute.Category)))
        for (GameFileViewModel* asset : _assetsList->assets().items())
            asset->resolve(EResolveCompute::Category);
        refreshFilters();
    }

    QList<QObject*> TreeItem::combinedEntries() const
    {
        QList<QObject*> combined;
        for (TreeItem* folder : _filteredFoldersView.items())
            combined.append(folder);
        for (GameFileViewModel* asset : _assetsList->assetsView().items())
            combined.append(asset);
        return combined;
    }

    QString TreeItem::toString() const
    {
        return QStringLiteral("%1 | %2 Folders | %3 Files")
            .arg(_header)
            .arg(_folders.count())
            .arg(_assetsList->assets().count());
    }

    AssetsFolderViewModel::AssetsFolderViewModel(QObject* parent)
        : QObject(parent), _folders(this), _foldersView(&_folders, byHeader)
    {
    }

    void AssetsFolderViewModel::setProjectName(const QString& projectName)
    {
        projectNameSlot() = projectName;
    }

    const QString& AssetsFolderViewModel::projectName()
    {
        return projectNameSlot();
    }

    void AssetsFolderViewModel::bulkPopulate(const QList<GameFile*>& entries)
    {
        if (entries.isEmpty())
            return;

        // C# builds the whole subtree detached, with every collection muted, and only then publishes it —
        // see the comment on the selection below for why the order matters.
        Framework::RangeObservableCollection<TreeItem*> treeItems;
        treeItems.setSuppressionState(true);

        // Environment.SpecialFolder.LocalApplicationData with the separators normalised to '/'. Qt's
        // GenericDataLocation is the same directory on Windows (%LOCALAPPDATA%) and already uses '/'.
        const QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

        for (GameFile* entry : entries)
        {
            TreeItem* lastNode = nullptr;
            TreeItem* parentItem = nullptr;

            QString path = QString::fromStdString(entry->Path());
            if (!localAppData.isEmpty() && path.startsWith(localAppData, Qt::CaseInsensitive))
            {
                path = path.mid(localAppData.size());
                while (path.startsWith(QLatin1Char('/')) || path.startsWith(QLatin1Char('\\')))
                    path.remove(0, 1);
            }

            const QStringList folders = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
            QString builder;
            Framework::RangeObservableCollection<TreeItem*>* parentNode = &treeItems;

            if (folders.size() <= 1)
            {
                // A file with no directory at all goes into a synthetic "Content" bucket.
                TreeItem* rootNode = findByHeaderOrNull(treeItems, QStringLiteral("Content"));
                if (rootNode == nullptr)
                {
                    rootNode = new TreeItem(QStringLiteral("Content"), entry, QStringLiteral("Content"),
                                            nullptr);
                    rootNode->setParent(this); // Parent is null, but something has to own it.
                    rootNode->folders().setSuppressionState(true);
                    rootNode->assetsList()->assets().setSuppressionState(true);
                    treeItems.add(rootNode);
                }

                rootNode->assetsList()->add(entry);
                continue;
            }

            for (qsizetype i = 0; i < folders.size() - 1; ++i)
            {
                const QString& folder = folders[i];
                builder.append(folder).append(QLatin1Char('/'));
                lastNode = findByHeaderOrNull(*parentNode, folder);

                if (lastNode == nullptr)
                {
                    const QString nodePath = builder.left(builder.size() - 1);
                    lastNode = new TreeItem(folder, entry, nodePath, parentItem);
                    if (parentItem == nullptr)
                        lastNode->setParent(this);
                    lastNode->folders().setSuppressionState(true);
                    lastNode->assetsList()->assets().setSuppressionState(true);
                    parentNode->add(lastNode);
                }

                parentItem = lastNode;
                parentNode = &lastNode->folders();
            }

            if (lastNode != nullptr)
                lastNode->assetsList()->add(entry);
        }

        _folders.addRange(treeItems.items());

        if (treeItems.count() > 0)
        {
            // Select after publishing the collection. Selecting a detached TreeItem lets WPF
            // auto-select the first root (usually the synthetic "Content" bucket) instead.
            const QString& project = projectName();
            TreeItem* selected = nullptr;
            for (TreeItem* item : treeItems.items())
            {
                if (!project.isEmpty() && item->header().compare(project, Qt::CaseInsensitive) == 0)
                {
                    selected = item;
                    break;
                }
            }
            if (selected == nullptr)
                selected = treeItems[0];
            selected->setIsSelected(true);
        }

        emit deferred(QStringLiteral("SearchVm.ChangeCollection"),
                      QStringLiteral("ViewModels/SearchViewModel"));

        for (TreeItem* folder : _folders.items())
            invokeOnCollectionChanged(folder);
    }

    void AssetsFolderViewModel::invokeOnCollectionChanged(TreeItem* item)
    {
        // Un-mutes the node depth-first and raises one Reset per non-empty collection, so every view
        // materialises exactly once instead of once per inserted item.
        item->folders().setSuppressionState(false);
        item->assetsList()->assets().setSuppressionState(false);

        if (item->folders().count() != 0)
        {
            item->folders().invokeOnCollectionChanged();

            for (TreeItem* folderItem : item->folders().items())
                invokeOnCollectionChanged(folderItem);
        }

        if (item->assetsList()->assets().count() != 0)
            item->assetsList()->assets().invokeOnCollectionChanged();
    }
}
