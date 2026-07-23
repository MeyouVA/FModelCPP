// Ported from FModel/ViewModels/ApplicationViewModel.cs
#include "ApplicationViewModel.h"

#include <QLocale>

#include "UE4/Versions/EGame.h"

#include "LoadingModesViewModel.h"
#include "../Constants.h"
#include "../Extensions/AssetCategoryExtensions.h"
#include "../Framework/FStatus.h"
#include "../Settings/DirectorySettings.h"
#include "../Settings/UserSettings.h"

namespace FModel::ViewModels
{
    using Framework::FStatus;

    ApplicationViewModel::ApplicationViewModel(QObject* parent)
        : ViewModel(parent)
        , _status(new FStatus(this))
        , _loadingModes(new LoadingModesViewModel(this))
        , _categories(Extensions::AssetCategoryExtensions::getBaseCategories())
    {
#ifdef NDEBUG
        setBuild(EBuildKind::Release);
#else
        setBuild(EBuildKind::Debug);
#endif

        // C# picks the current directory here (AvoidEmptyGameDirectory) and hard-exits when the user cancels.
        // The directory selector is not ported; see the header note.

        _status->setStatus(EStatusKind::Ready);
    }

    void ApplicationViewModel::setBuild(EBuildKind value)
    {
        if (setProperty(_build, value, QStringLiteral("Build")))
            raisePropertyChanged(QStringLiteral("TitleExtra"));
    }

    void ApplicationViewModel::setIsAssetsExplorerVisible(bool value)
    {
        if (value && !Settings::UserSettings::Default()->featurePreviewNewAssetExplorer())
            return;

        setProperty(_isAssetsExplorerVisible, value, QStringLiteral("IsAssetsExplorerVisible"));
    }

    void ApplicationViewModel::setSelectedLeftTabIndex(int value)
    {
        if (value < 0 || value > 2) return;
        setProperty(_selectedLeftTabIndex, value, QStringLiteral("SelectedLeftTabIndex"));
    }

    QString ApplicationViewModel::initialWindowTitle() const
    {
        // C#: $"FModel ({APP_SHORT_COMMIT_ID} - {APP_BUILD_DATE:MMM d, yyyy})"
        const QString date = QLocale::c().toString(Constants::APP_BUILD_DATE(), QStringLiteral("MMM d, yyyy"));
        return QStringLiteral("FModel (%1 - %2)").arg(Constants::APP_SHORT_COMMIT_ID(), date);
    }

    QString ApplicationViewModel::gameDisplayName() const
    {
        // C#: CUE4Parse.Provider.GameDisplayName ?? "Unknown". CUE4ParseViewModel is not ported yet, so this
        // is always the null branch for now.
        return QStringLiteral("Unknown");
    }

    QString ApplicationViewModel::titleExtra() const
    {
        // C#: $"({CurrentDir.UeVersion}){(Build != Release ? $" ({Build})" : "")}"
        const auto* currentDir = Settings::UserSettings::Default()->currentDir();
        const char* version = currentDir
            ? CUE4Parse::UE4::Versions::EGameName(currentDir->ueVersion())
            : nullptr;

        QString extra = QStringLiteral("(%1)").arg(version ? QString::fromLatin1(version) : QString());
        if (_build != EBuildKind::Release)
            extra += QStringLiteral(" (%1)").arg(buildKindName(_build));
        return extra;
    }
}
