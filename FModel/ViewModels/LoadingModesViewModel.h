#pragma once
// Ported from FModel/ViewModels/LoadingModesViewModel.cs — the list of ELoadingMode values the toolbar's
// loading-mode picker binds to.
//
// Deliberate differences from C#:
//   * ReadOnlyObservableCollection<ELoadingMode> becomes a plain const QList: the collection is built once in
//     the constructor and never mutated, so the observability is unused.
//   * C# fills it with Enum.GetValues<ELoadingMode>(). C++ has no such reflection, so enumerateLoadingModes()
//     writes the members out; a static_assert in the .cpp ties the list back to the enum.
//   * LoadCommand is not ported yet (it drives CUE4ParseViewModel, which is a later slice). TODO: add it with
//     the command layer.

#include <QList>

#include "../Enums.h"
#include "../Framework/ViewModel.h"

namespace FModel::ViewModels
{
    class LoadingModesViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit LoadingModesViewModel(QObject* parent = nullptr);

        const QList<ELoadingMode>& modes() const { return _modes; }

    private:
        static QList<ELoadingMode> enumerateLoadingModes();

        QList<ELoadingMode> _modes;
    };
}
