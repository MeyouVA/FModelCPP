// Ported from FModel/ViewModels/Commands/RightClickMenuCommand.cs
#include "RightClickMenuCommand.h"

#include <algorithm>
#include <stdexcept>

#include "FileProvider/Objects/GameFile.h"

#include "../../Settings/UserSettings.h"
#include "../AssetsFolderViewModel.h"

namespace FModel::ViewModels::Commands
{
    using CUE4Parse::FileProvider::Objects::GameFile;

    RightClickMenuCommand::FTriggerAction RightClickMenuCommand::resolveTrigger(const QString& trigger)
    {
        if (trigger == QStringLiteral("Assets_Extract_New_Tab"))
            return {EAction::Show, EShowAssetType::JSON, EBulkType::None};
        if (trigger == QStringLiteral("Assets_Show_Metadata"))
            return {EAction::Show, EShowAssetType::Metadata, EBulkType::None};
        if (trigger == QStringLiteral("Assets_Show_References"))
            return {EAction::Show, EShowAssetType::References, EBulkType::None};
        if (trigger == QStringLiteral("Assets_Decompile"))
            return {EAction::Show, EShowAssetType::Decompile, EBulkType::Code};

        if (trigger == QStringLiteral("Save_Data"))
            return {EAction::Export, EShowAssetType::None, EBulkType::Raw};
        if (trigger == QStringLiteral("Save_Properties"))
            return {EAction::Export, EShowAssetType::None, EBulkType::Properties};
        if (trigger == QStringLiteral("Save_Textures"))
            return {EAction::Export, EShowAssetType::None, EBulkType::Textures};
        if (trigger == QStringLiteral("Save_Models"))
            return {EAction::Export, EShowAssetType::None, EBulkType::Meshes};
        if (trigger == QStringLiteral("Save_Animations"))
            return {EAction::Export, EShowAssetType::None, EBulkType::Animations};
        if (trigger == QStringLiteral("Save_Audio"))
            return {EAction::Export, EShowAssetType::None, EBulkType::Audio};
        if (trigger == QStringLiteral("Save_Code"))
            return {EAction::Export, EShowAssetType::None, EBulkType::Code};

        throw std::out_of_range("Unsupported asset action.");
    }

    RightClickMenuCommand::FBulkTarget RightClickMenuCommand::resolveBulkTarget(EBulkType bulkType)
    {
        // The switch runs on the trigger's own bulk type, before EBulkType::Auto is folded in, so plain
        // equality is what C# compares too.
        const auto* settings = Settings::UserSettings::Default();
        switch (bulkType)
        {
            case EBulkType::Raw:        return {settings->rawDataDirectory(), QStringLiteral("files")};
            case EBulkType::Properties: return {settings->propertiesDirectory(), QStringLiteral("json files")};
            case EBulkType::Textures:   return {settings->textureDirectory(), QStringLiteral("textures")};
            case EBulkType::Meshes:     return {settings->modelDirectory(), QStringLiteral("models")};
            case EBulkType::Animations: return {settings->modelDirectory(), QStringLiteral("animations")};
            case EBulkType::Audio:      return {settings->audioDirectory(), QStringLiteral("audio files")};
            case EBulkType::Code:       return {settings->codeDirectory(), QStringLiteral("code files")};
            default:                    return {};  // C#'s `_ => (null, null)`
        }
    }

    QList<RightClickMenuCommand::FAssetGroup> RightClickMenuCommand::groupAssets(
        const QList<GameFile*>& assets, EBulkType bulkType)
    {
        QList<FAssetGroup> groups;
        for (GameFile* asset : assets)
        {
            const QString directory = QString::fromStdString(asset->Directory());

            auto it = std::find_if(groups.begin(), groups.end(),
                                   [&directory](const FAssetGroup& g) { return g.Directory == directory; });
            if (it == groups.end())
            {
                groups.append(FAssetGroup{directory, {asset}, false, EBulkType::None});
            }
            else
            {
                it->Assets.append(asset);
            }
        }

        // C# computes these inside the foreach over the groups; they depend only on the group, so they are
        // filled in here instead.
        for (FAssetGroup& group : groups)
        {
            group.Update = group.Assets.size() > 1;
            group.Bulk = bulkType | (group.Update ? EBulkType::Auto : EBulkType::None);
        }
        return groups;
    }

    QString RightClickMenuCommand::exportPath(const QString& dirType, const QString& directory)
    {
        // SubstringAfterLast('/'): the whole string when there is no '/'.
        const QString tail = Settings::UserSettings::Default()->keepDirectoryStructure()
            ? directory
            : directory.mid(directory.lastIndexOf(QLatin1Char('/')) + 1);

        // Path.Combine: a rooted second part wins outright, otherwise the parts are joined with the platform
        // separator (never doubling one the first part already ends with).
        QString combined;
        const bool tailIsRooted = tail.startsWith(QLatin1Char('/')) || tail.startsWith(QLatin1Char('\\')) ||
                                  (tail.size() > 1 && tail[1] == QLatin1Char(':'));
        if (dirType.isEmpty() || tailIsRooted)
        {
            combined = tail;
        }
        else
        {
            combined = dirType;
            if (!combined.endsWith(QLatin1Char('/')) && !combined.endsWith(QLatin1Char('\\')))
                combined += QLatin1Char('\\');
            combined += tail;
        }

        return combined.replace(QLatin1Char('\\'), QLatin1Char('/'));
    }

    RightClickMenuCommand::FSelection RightClickMenuCommand::splitSelection(const QVariantList& selection)
    {
        // C# runs two passes over the same array: `param.OfType<TreeItem>()` and a Select/Where that maps
        // GameFile through and GameFileViewModel to its .Asset. One pass produces the same two lists in the
        // same order.
        FSelection split;
        for (const QVariant& item : selection)
        {
            if (auto* folder = item.value<TreeItem*>())
            {
                split.Folders.append(folder);
            }
            else if (auto* gf = item.value<GameFile*>())
            {
                split.Assets.append(gf); // Search view passes GameFile directly
            }
            else if (auto* gvm = item.value<GameFileViewModel*>())
            {
                split.Assets.append(gvm->asset());
            }
        }
        return split;
    }

    void RightClickMenuCommand::execute(ApplicationViewModel* /*contextViewModel*/, const QVariant& parameter)
    {
        // C#: `if (parameter is not object[] parameters || parameters[0] is not string trigger) return;`
        const QVariantList parameters = parameter.toList();
        if (parameters.size() < 2 || parameters[0].typeId() != QMetaType::QString)
            return;

        const QString trigger = parameters[0].toString();

        const QVariantList param = parameters[1].toList();
        if (param.isEmpty())
            return;

        const FSelection selection = splitSelection(param);

        if (selection.Folders.isEmpty() && selection.Assets.isEmpty())
            return;

        // Throws on an unknown trigger before any work starts, as in C#.
        const FTriggerAction action = resolveTrigger(trigger);
        (void)action;

        // What follows in C# is Interlocked.Exchange on CUE4Parse.ExportedCount / FailedExportCount and then
        // ThreadWorkerView.Begin(...) running the show or export walk — CUE4ParseViewModel,
        // ThreadWorkerViewModel and FLogger, none of them ported. resolveBulkTarget / groupAssets /
        // exportPath above are that walk's arithmetic, already in place for it: the folder loop pairs
        // ExportFolder (Raw) or ExtractFolder (everything else, with Auto folded in) with
        // exportPath(dirType, folder->pathAtThisPoint()).
        emit deferred(trigger, static_cast<int>(selection.Assets.size()));
    }
}
