// Ported from FModel/ViewModels/AssetsListViewModel.cs
#include "AssetsListViewModel.h"

#include "FileProvider/Objects/GameFile.h"

namespace FModel::ViewModels
{
    using CUE4Parse::FileProvider::Objects::GameFile;

    AssetsListViewModel::AssetsListViewModel(QObject* parent)
        : QObject(parent),
          _assets(this),
          // SortDescription("Asset.Path", ListSortDirection.Ascending). C# compares with the default string
          // comparer, which is ordinal here (WPF sorts with Comparer<string>.Default == ordinal).
          _assetsView(&_assets, [](GameFileViewModel* a, GameFileViewModel* b)
          {
              return a->asset()->Path() < b->asset()->Path();
          })
    {
    }

    void AssetsListViewModel::add(GameFile* gameFile)
    {
        _assets.add(new GameFileViewModel(gameFile, this));
    }
}
