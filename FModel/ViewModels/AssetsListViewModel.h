#pragma once
// Ported from FModel/ViewModels/AssetsListViewModel.cs — the flat list of assets sitting in one tree folder.
//
// Deliberate differences from C#:
//   * The collection holds GameFileViewModel* rather than GameFileViewModel, and each row is parented to
//     this view-model, so the list owns its rows (C# has the GC).
//   * The `new SortDescription("Asset.Path", Ascending)` property path becomes an explicit comparison; see
//     Framework/CollectionView.h for why.
//   * `AssetsView` is created eagerly. C#'s `??=` exists to keep the ListCollectionView off the worker
//     thread until the UI asks for it; there is no such constraint here, and the constructor immediately
//     assigns the view's Filter anyway, which forces it in C# too.

#include <QObject>

#include "GameFileViewModel.h"
#include "../Framework/CollectionView.h"
#include "../Framework/RangeObservableCollection.h"

namespace CUE4Parse::FileProvider::Objects { class GameFile; }

namespace FModel::ViewModels
{
    class AssetsListViewModel : public QObject
    {
        Q_OBJECT

    public:
        explicit AssetsListViewModel(QObject* parent = nullptr);

        Framework::RangeObservableCollection<GameFileViewModel*>& assets() { return _assets; }
        const Framework::RangeObservableCollection<GameFileViewModel*>& assets() const { return _assets; }

        Framework::CollectionView<GameFileViewModel*>& assetsView() { return _assetsView; }
        const Framework::CollectionView<GameFileViewModel*>& assetsView() const { return _assetsView; }

        // C#: `public void Add(GameFile gameFile) => Assets.Add(new GameFileViewModel(gameFile));`
        void add(CUE4Parse::FileProvider::Objects::GameFile* gameFile);

    private:
        Framework::RangeObservableCollection<GameFileViewModel*> _assets;
        Framework::CollectionView<GameFileViewModel*> _assetsView;
    };
}
