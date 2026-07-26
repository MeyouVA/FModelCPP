#pragma once
// Ported from FModel/ViewModels/GameFileViewModel.cs — one row in the asset list: the GameFile plus what the
// explorer has managed to work out about it (its category, which bulk actions apply, its display type).
//
// C# resolves a row two ways. `ResolveByExtensionAsync` keys off the file extension alone — that half is
// ported here in full. `ResolveByPackageAsync` loads the asset as a UE package, constructs its main export
// and runs a ~60-arm type switch over it; that half needs CUE4ParseViewModel (unported), the whole concrete
// UExport class tree, and — for the two preview arms — CUE4Parse-Conversion's texture decoder and Skia. It is
// deferred: resolve() raises deferred() and marks the row resolved, exactly where C# would have queued the
// package work.
//
// Deliberate differences from C#:
//   * Everything is synchronous. C# returns Task and runs the package half on the thread pool; there is no
//     threading layer in the port yet, and the extension half never blocked anyway. `resolve()` keeps C#'s
//     try/catch contract: on failure the row is marked EResolveCompute::All so it is never retried.
//   * `PreviewImage` (an ImageSource) and `NumTextures` are omitted. Both are written only from the two
//     decode arms of the package half, so nothing in the ported subset could ever set them.
//   * C# reaches the app through `ApplicationService.ApplicationView`, a process-wide service locator that
//     is itself unported. Two statics stand in for the two things it is asked for, and they are static for
//     the same reason C#'s locator is global: `setApplicationView` (the real, ported ApplicationViewModel —
//     `IsAssetsExplorerVisible` gates the preview half) and `setGameVersion` (which C# reads through the
//     unported CUE4ParseViewModel, so there is no object to hang it off yet; nullopt == C#'s null, no
//     provider loaded). The game-specific extension arms consult the latter exactly as C# does.
//   * `ExtractAsync` needs ThreadWorkerViewModel + CUE4ParseViewModel; it raises deferred().
//   * The GameFile is NOT owned. C# holds a reference into the provider's file map, and so does this.

#include <optional>

#include <QObject>
#include <QString>

#include "UE4/Versions/EGame.h"

#include "../Enums.h"
#include "../Framework/ViewModel.h"

class QTimer;

namespace CUE4Parse::FileProvider::Objects { class GameFile; }

namespace FModel::ViewModels
{
    class ApplicationViewModel;

    // Declared at the bottom of GameFileViewModel.cs, in this namespace.
    enum class EResolveCompute
    {
        None = 0,
        Category = 1 << 0,
        Preview = 1 << 1,

        All = Category | Preview,
    };

    inline constexpr EResolveCompute operator|(EResolveCompute a, EResolveCompute b)
    { return static_cast<EResolveCompute>(static_cast<int>(a) | static_cast<int>(b)); }
    inline constexpr EResolveCompute operator&(EResolveCompute a, EResolveCompute b)
    { return static_cast<EResolveCompute>(static_cast<int>(a) & static_cast<int>(b)); }
    // C#'s `~` on a flags enum operates on the underlying int and does NOT mask to the declared bits. One
    // arm of ResolveByExtensionAsync depends on that; see the comment at its site.
    inline constexpr EResolveCompute operator~(EResolveCompute a)
    { return static_cast<EResolveCompute>(~static_cast<int>(a)); }
    inline constexpr EResolveCompute& operator|=(EResolveCompute& a, EResolveCompute b)
    { a = a | b; return a; }
    inline constexpr EResolveCompute& operator&=(EResolveCompute& a, EResolveCompute b)
    { a = a & b; return a; }
    inline constexpr bool hasFlag(EResolveCompute value, EResolveCompute flag)
    { return (static_cast<int>(value) & static_cast<int>(flag)) == static_cast<int>(flag); }

    class GameFileViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit GameFileViewModel(CUE4Parse::FileProvider::Objects::GameFile* asset, QObject* parent = nullptr);

        // See the header note: the two halves of C#'s ApplicationService.ApplicationView lookup.
        static void setApplicationView(ApplicationViewModel* applicationView);
        static ApplicationViewModel* applicationView();
        static void setGameVersion(std::optional<CUE4Parse::UE4::Versions::EGame> game);
        static std::optional<CUE4Parse::UE4::Versions::EGame> gameVersion();

        CUE4Parse::FileProvider::Objects::GameFile* asset() const { return _asset; }

        EResolveCompute resolved() const { return _resolved; }

        // Starts as the raw extension (C#: `private string _resolvedAssetType = asset.Extension;`).
        const QString& resolvedAssetType() const { return _resolvedAssetType; }

        bool isSelected() const { return _isSelected; }
        void setIsSelected(bool value);

        EAssetCategory assetCategory() const { return _assetCategory; }
        EBulkType assetActions() const { return _assetActions; }

        // C#'s ResolveAsync: catches everything, marks the row fully resolved on failure.
        void resolve(EResolveCompute compute);

        // C#'s ExtractAsync — raises deferred().
        void extract();

        // C#'s OnIsVisible: a 100 ms debounce, each call cancelling the previous one, after which the row
        // resolves everything. Modelled with a restarting single-shot QTimer.
        void onIsVisible();

    signals:
        // Raised where C# would have run the package half or handed work to the ThreadWorker.
        void deferred(const QString& what, const QString& waitingOn);

    private:
        void setResolvedAssetType(const QString& value);
        // C#'s AssetCategory setter: assigning it ALWAYS marks Category resolved, even when the value is
        // unchanged ("blindly assume category is resolved when set").
        void setAssetCategory(EAssetCategory value);
        void setAssetActions(EBulkType value);

        void resolveInternal(EResolveCompute compute);
        void resolveByExtension(EResolveCompute compute);
        void resolveByPackage(EResolveCompute compute);

        CUE4Parse::FileProvider::Objects::GameFile* _asset = nullptr;
        EResolveCompute _resolved = EResolveCompute::None;
        QString _resolvedAssetType;
        bool _isSelected = false;
        EAssetCategory _assetCategory = EAssetCategory::All;
        EBulkType _assetActions = EBulkType::None;

        QTimer* _previewTimer = nullptr; // C#'s _previewCts
    };
}
