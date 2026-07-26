// Ported from FModel/ViewModels/GameFileViewModel.cs
#include "GameFileViewModel.h"

#include <QPointer>
#include <QTimer>

#include "FileProvider/Objects/GameFile.h"

#include "ApplicationViewModel.h"
#include "../Settings/UserSettings.h"

namespace FModel::ViewModels
{
    using CUE4Parse::FileProvider::Objects::GameFile;
    using CUE4Parse::UE4::Versions::EGame;

    namespace
    {
        // Function-local statics keep the two seams out of the static-init order, as elsewhere in the port.
        QPointer<ApplicationViewModel>& applicationViewSlot()
        {
            static QPointer<ApplicationViewModel> view;
            return view;
        }

        std::optional<EGame>& gameVersionSlot()
        {
            static std::optional<EGame> game;
            return game;
        }
    }

    void GameFileViewModel::setApplicationView(ApplicationViewModel* applicationView)
    {
        applicationViewSlot() = applicationView;
    }

    ApplicationViewModel* GameFileViewModel::applicationView()
    {
        return applicationViewSlot().data();
    }

    void GameFileViewModel::setGameVersion(std::optional<EGame> game)
    {
        gameVersionSlot() = game;
    }

    std::optional<EGame> GameFileViewModel::gameVersion()
    {
        return gameVersionSlot();
    }

    GameFileViewModel::GameFileViewModel(GameFile* asset, QObject* parent)
        : ViewModel(parent), _asset(asset)
    {
        if (_asset != nullptr)
            _resolvedAssetType = QString::fromStdString(_asset->Extension());
    }

    void GameFileViewModel::setIsSelected(bool value)
    {
        setProperty(_isSelected, value, QStringLiteral("IsSelected"));
    }

    void GameFileViewModel::setResolvedAssetType(const QString& value)
    {
        setProperty(_resolvedAssetType, value, QStringLiteral("ResolvedAssetType"));
    }

    void GameFileViewModel::setAssetCategory(EAssetCategory value)
    {
        setProperty(_assetCategory, value, QStringLiteral("AssetCategory"));
        // Outside the `if changed` guard on purpose — C#: "blindly assume category is resolved when set,
        // even if unchanged". Several extension arms rely on it (the `default:` one sets All just for this).
        _resolved |= EResolveCompute::Category;
    }

    void GameFileViewModel::setAssetActions(EBulkType value)
    {
        setProperty(_assetActions, value, QStringLiteral("AssetActions"));
    }

    void GameFileViewModel::resolve(EResolveCompute compute)
    {
        // C# wraps ResolveInternalAsync in a try/catch that logs, marks the row fully resolved so it is
        // never retried, and swallows the exception.
        try
        {
            resolveInternal(compute);
        }
        catch (...)
        {
            _resolved = EResolveCompute::All;
        }
    }

    void GameFileViewModel::resolveInternal(EResolveCompute compute)
    {
        const ApplicationViewModel* view = applicationView();
        if (view == nullptr || !view->isAssetsExplorerVisible() ||
            !Settings::UserSettings::Default()->previewTexturesAssetExplorer())
        {
            compute &= ~EResolveCompute::Preview;
        }

        compute &= ~_resolved;
        if (compute == EResolveCompute::None)
            return;

        // C#: `if (!Asset.IsUePackage || _applicationView.CUE4Parse is null)`. CUE4ParseViewModel is
        // unported, so the second half is permanently true — but the first half is what decides which
        // resolver a row would eventually get, so it is spelled out rather than collapsed.
        if (_asset == nullptr || !_asset->IsUePackage())
        {
            resolveByExtension(compute);
            return;
        }

        resolveByPackage(compute);
    }

    void GameFileViewModel::resolveByPackage(EResolveCompute compute)
    {
        // The two extension-keyed shortcuts C# takes before touching the provider are portable in full.
        const QString extension = QString::fromStdString(_asset->Extension());
        if (extension == QStringLiteral("umap"))
        {
            setAssetCategory(EAssetCategory::World);
            setAssetActions(EBulkType::Meshes | EBulkType::Textures | EBulkType::Audio | EBulkType::Code);
            setResolvedAssetType(QStringLiteral("World"));
            _resolved |= EResolveCompute::Preview;
            return;
        }
        if (QString::fromStdString(_asset->NameWithoutExtension()).endsWith(QStringLiteral("_BuiltData")))
        {
            setAssetCategory(EAssetCategory::BuildData);
            setAssetActions(EBulkType::Textures);
            setResolvedAssetType(QStringLiteral("MapBuildDataRegistry"));
            _resolved |= EResolveCompute::Preview;
            return;
        }

        // Everything past this point in C# loads the package, resolves its main export, constructs it and
        // runs a ~60-arm type switch (plus two decode-and-preview arms). That needs CUE4ParseViewModel, the
        // concrete UExport tree and the texture decoder. The row is marked resolved so the explorer does not
        // spin on it, which is also what C#'s failure path does.
        (void)compute;
        _resolved = EResolveCompute::All;
        emit deferred(QString::fromStdString(_asset->Path()),
                      QStringLiteral("ViewModels/CUE4ParseViewModel (ResolveByPackageAsync)"));
    }

    void GameFileViewModel::resolveByExtension(EResolveCompute compute)
    {
        _resolved |= EResolveCompute::Preview;

        const QString lowercaseExtension =
            _asset != nullptr ? QString::fromStdString(_asset->Extension()).toLower() : QString();

        // C#'s switch, arm for arm and in the same order. The `when GameVersion is ...` guards at the end
        // read the seam described in the header.
        static const QStringList dataExtensions{
            QStringLiteral("uproject"), QStringLiteral("uefnproject"), QStringLiteral("upluginmanifest"),
            QStringLiteral("uplugin"), QStringLiteral("ini"), QStringLiteral("locmeta"),
            QStringLiteral("locres"), QStringLiteral("verse"), QStringLiteral("lua"),
            QStringLiteral("luac"), QStringLiteral("json5"), QStringLiteral("json"),
            QStringLiteral("bin"), QStringLiteral("txt"), QStringLiteral("log"),
            QStringLiteral("pem"), QStringLiteral("xml"), QStringLiteral("gitignore"),
            QStringLiteral("gitattributes"), QStringLiteral("html"), QStringLiteral("css"),
            QStringLiteral("js"), QStringLiteral("data"), QStringLiteral("csv"),
            QStringLiteral("sql"), QStringLiteral("py"), QStringLiteral("cs")};
        static const QStringList byteCodeExtensions{
            QStringLiteral("stinfo"), QStringLiteral("ushaderbytecode"), QStringLiteral("upipelinecache"),
            QStringLiteral("dxbc")};
        // "awb" is technically a soundbank; upstream keeps it here so it reads differently from "acb".
        static const QStringList audioExtensions{
            QStringLiteral("wav"), QStringLiteral("awb"), QStringLiteral("xvag"), QStringLiteral("flac"),
            QStringLiteral("at9"), QStringLiteral("wem"), QStringLiteral("ogg")};
        static const QStringList soundBankExtensions{
            QStringLiteral("acb"), QStringLiteral("bank"), QStringLiteral("bnk"), QStringLiteral("pck")};
        static const QStringList fontExtensions{
            QStringLiteral("ufont"), QStringLiteral("otf"), QStringLiteral("ttf")};
        static const QStringList imageExtensions{
            QStringLiteral("jpg"), QStringLiteral("png"), QStringLiteral("bmp"), QStringLiteral("svg")};

        const std::optional<EGame> game = gameVersion();

        if (dataExtensions.contains(lowercaseExtension))
        {
            setAssetCategory(EAssetCategory::Data);
        }
        else if (byteCodeExtensions.contains(lowercaseExtension))
        {
            setAssetCategory(EAssetCategory::ByteCode);
        }
        else if (audioExtensions.contains(lowercaseExtension))
        {
            setAssetCategory(EAssetCategory::Audio);
            setAssetActions(EBulkType::Audio);
        }
        else if (soundBankExtensions.contains(lowercaseExtension))
        {
            setAssetCategory(EAssetCategory::SoundBank);
            setAssetActions(EBulkType::Audio);
        }
        else if (fontExtensions.contains(lowercaseExtension))
        {
            setAssetCategory(EAssetCategory::Font);
        }
        else if (lowercaseExtension == QStringLiteral("mp4"))
        {
            setAssetCategory(EAssetCategory::Video);
        }
        else if (imageExtensions.contains(lowercaseExtension))
        {
            // Upstream slip, kept: `Resolved |= ~EResolveCompute.Preview`. C#'s `~` runs on the underlying
            // int and is NOT masked to the declared bits, so this ORs in every bit EXCEPT Preview — leaving
            // Resolved at ~0 once the category setter adds Category back. The visible consequence is that
            // `Resolved == EResolveCompute.All` is then false, so OnIsVisible never takes its early return
            // for an image row. A `&= ~` was presumably meant.
            _resolved |= ~EResolveCompute::Preview;
            setAssetCategory(EAssetCategory::Texture);
            setAssetActions(EBulkType::Textures);
            if (!hasFlag(compute, EResolveCompute::Preview))
                return;

            // C# decodes the bytes here (SkiaSharp, plus SKSvg for "svg") and hands a frozen BitmapImage to
            // the UI thread. No image pipeline is ported.
            emit deferred(QString::fromStdString(_asset->Path()),
                          QStringLiteral("Skia image preview (ResolveByExtensionAsync)"));
        }
        else if (lowercaseExtension == QStringLiteral("ace") && game == EGame::GAME_Borderlands3)
        {
            setAssetCategory(EAssetCategory::Borderlands);
        }
        else if (lowercaseExtension == QStringLiteral("ncs") && game == EGame::GAME_Borderlands4)
        {
            setAssetCategory(EAssetCategory::Borderlands);
        }
        else if (lowercaseExtension == QStringLiteral("dat") && game == EGame::GAME_Aion2)
        {
            setAssetCategory(EAssetCategory::Aion2);
        }
        else if ((lowercaseExtension == QStringLiteral("bytes") || lowercaseExtension == QStringLiteral("non") ||
                  lowercaseExtension == QStringLiteral("cam")) && game == EGame::GAME_RocoKingdomWorld)
        {
            setAssetCategory(EAssetCategory::RocoKingdomWorld);
        }
        else if (lowercaseExtension == QStringLiteral("ustbin") && game == EGame::GAME_DeltaForce)
        {
            setAssetCategory(EAssetCategory::DeltaForce);
        }
        else
        {
            setAssetCategory(EAssetCategory::All); // just so it sets resolved
        }
    }

    void GameFileViewModel::extract()
    {
        // ThreadWorkerView.Begin(ct => CUE4Parse.ExtractSelected(ct, [Asset]))
        emit deferred(_asset != nullptr ? QString::fromStdString(_asset->Path()) : QString(),
                      QStringLiteral("ViewModels/CUE4ParseViewModel (ExtractSelected)"));
    }

    void GameFileViewModel::onIsVisible()
    {
        if (_resolved == EResolveCompute::All)
            return;

        // C# cancels the pending CancellationTokenSource and starts a new 100 ms delay; restarting a
        // single-shot timer is the same thing, and a QTimer parented here dies with the row.
        if (_previewTimer == nullptr)
        {
            _previewTimer = new QTimer(this);
            _previewTimer->setSingleShot(true);
            connect(_previewTimer, &QTimer::timeout, this, [this] { resolve(EResolveCompute::All); });
        }
        _previewTimer->start(100);
    }
}
