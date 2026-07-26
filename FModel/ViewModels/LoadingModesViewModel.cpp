// Ported from FModel/ViewModels/LoadingModesViewModel.cs
#include "LoadingModesViewModel.h"

#include "Commands/LoadCommand.h"

namespace FModel::ViewModels
{
    LoadingModesViewModel::LoadingModesViewModel(QObject* parent)
        : ViewModel(parent), _modes(enumerateLoadingModes())
    {
    }

    Commands::LoadCommand* LoadingModesViewModel::loadCommand()
    {
        if (!_loadCommand)
            _loadCommand = new Commands::LoadCommand(this, this);
        return _loadCommand;
    }

    QList<ELoadingMode> LoadingModesViewModel::enumerateLoadingModes()
    {
        // Stands in for C#'s Enum.GetValues<ELoadingMode>(), in declaration order.
        static_assert(static_cast<int>(ELoadingMode::AllButPatched) == 4,
                      "ELoadingMode gained a member; extend enumerateLoadingModes()");
        return {ELoadingMode::Multiple, ELoadingMode::All, ELoadingMode::AllButNew,
                ELoadingMode::AllButModified, ELoadingMode::AllButPatched};
    }
}
