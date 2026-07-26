// Ported from FModel/ViewModels/Commands/LoadCommand.cs
#include "LoadCommand.h"

#include <map>

#include "FileProvider/Objects/OsGameFile.h"
#include "UE4/VirtualFileSystem/VfsEntry.h"

#include "../../Framework/FStatus.h"
#include "../../Settings/UserSettings.h"
#include "../ApplicationViewModel.h"
#include "../AssetsFolderViewModel.h"
#include "../CUE4ParseViewModel.h"
#include "../GameDirectoryViewModel.h"
#include "../ThreadWorkerViewModel.h"

namespace FModel::ViewModels::Commands
{
    using CUE4Parse::FileProvider::Objects::GameFile;
    using CUE4Parse::FileProvider::Objects::OsGameFile;
    using CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider;
    using CUE4Parse::UE4::VirtualFileSystem::VfsEntry;

    QString LoadCommand::packageCountLabel(int count)
    {
        // C#'s "### ### ###" custom format: digits grouped in threes, separated by spaces, no leading zeros.
        QString digits = QString::number(count);
        for (int at = digits.size() - 3; at > 0; at -= 3)
            digits.insert(at, QLatin1Char(' '));
        return QStringLiteral("%1 Packages").arg(digits);
    }

    QList<GameFile*> LoadCommand::filterDirectoryFiles(AbstractVfsFileProvider& provider,
                                                       const QList<FileItem*>* directoryFiles)
    {
        QSet<QString> filter;
        bool includeLooseFiles = false;
        const bool hasList = directoryFiles != nullptr;
        if (hasList)
        {
            for (const FileItem* directoryFile : *directoryFiles)
            {
                // A container that never mounted cannot contribute anything, so it is skipped even when
                // the user ticked it.
                if (!directoryFile->isEnabled())
                    continue;
                if (directoryFile->isLooseFilesContainer())
                {
                    includeLooseFiles = true;
                    continue;
                }
                filter.insert(directoryFile->name());
            }
        }

        const bool hasFilter = hasList && !filter.isEmpty();
        const bool hasSelection = hasFilter || includeLooseFiles;

        QList<GameFile*> entries;
        provider.Files.ForEach([&](const std::string&, const std::shared_ptr<GameFile>& asset)
        {
            // A .uexp/.ubulk is part of its .uasset, never a row of its own.
            if (asset->IsUePackagePayload())
                return;

            if (!hasSelection)
            {
                entries.append(asset.get());
                return;
            }

            if (auto* entry = dynamic_cast<VfsEntry*>(asset.get());
                entry != nullptr && entry->Vfs != nullptr &&
                filter.contains(QString::fromStdString(entry->Vfs->Name())))
            {
                entries.append(asset.get());
            }
            else if (includeLooseFiles && dynamic_cast<OsGameFile*>(asset.get()) != nullptr)
            {
                entries.append(asset.get());
            }
        });
        return entries;
    }

    QList<GameFile*> LoadCommand::filterPatchedFiles(AbstractVfsFileProvider& provider)
    {
        // C# keys a Dictionary with the provider's PathComparer and keeps, per path, the entry whose
        // archive has the HIGHEST ReadOrder — a later patch pak shadowing the base game's copy.
        std::map<std::string, GameFile*> loaded;
        provider.Files.ForEach([&](const std::string& key, const std::shared_ptr<GameFile>& asset)
        {
            if (asset->IsUePackagePayload())
                return;

            auto existing = loaded.find(key);
            if (existing != loaded.end())
            {
                auto* entry = dynamic_cast<VfsEntry*>(asset.get());
                auto* existingEntry = dynamic_cast<VfsEntry*>(existing->second);
                if (entry != nullptr && existingEntry != nullptr && entry->Vfs != nullptr &&
                    existingEntry->Vfs != nullptr && entry->Vfs->ReadOrder() < existingEntry->Vfs->ReadOrder())
                {
                    return;
                }
            }

            loaded[key] = asset.get();
        });

        QList<GameFile*> entries;
        entries.reserve(static_cast<qsizetype>(loaded.size()));
        for (const auto& [key, asset] : loaded)
            entries.append(asset);
        return entries;
    }

    void LoadCommand::execute(LoadingModesViewModel* /*contextViewModel*/, const QVariant& parameter)
    {
        if (_applicationView == nullptr || _applicationView->cue4Parse() == nullptr)
            return;

        CUE4ParseViewModel* cue4Parse = _applicationView->cue4Parse();
        AbstractVfsFileProvider* provider = cue4Parse->provider();
        if (provider == nullptr)
            return;

        if (provider->Keys().empty() && !provider->RequiredKeys().empty())
        {
            emit refused(QStringLiteral("An encrypted archive has been found. In order to decrypt it, please "
                                        "specify a working AES encryption key"));
            return;
        }
        if (provider->Files.Count() == 0)
        {
            emit refused(QStringLiteral("No files were found in the archives or the specified directory"));
            return;
        }

        cue4Parse->assetsFolder()->folders().clear();
        // C# also clears SearchVm.SearchResults and closes the search window here.
        _applicationView->setSelectedLeftTabIndex(1); // folders tab
        _applicationView->setIsAssetsExplorerVisible(true);

        auto* settings = Settings::UserSettings::Default();
        Framework::FStatus* status = _applicationView->status();

        _applicationView->threadWorker()->begin([&](FCancellationToken& token)
        {
            status->updateStatusLabel(QStringLiteral("Packages"), QStringLiteral("Filtering"));

            QList<GameFile*> entries;
            ELoadingMode mode = settings->loadingMode();

            if (mode == ELoadingMode::Multiple)
            {
                // The parameter is the Archives tab's selection.
                QList<FileItem*> selected;
                for (const QVariant& item : parameter.toList())
                {
                    if (auto* file = item.value<FileItem*>())
                        selected.append(file);
                }

                if (selected.isEmpty())
                {
                    // C#'s `goto case ELoadingMode.All`, which also WRITES All back into settings — so an
                    // empty-selection load silently changes the user's loading mode for good.
                    settings->setLoadingMode(ELoadingMode::All);
                    mode = ELoadingMode::All;
                }
                else
                {
                    token.throwIfCancellationRequested();
                    entries = filterDirectoryFiles(*provider, &selected);
                }
            }

            if (mode == ELoadingMode::All)
            {
                token.throwIfCancellationRequested();
                entries = filterDirectoryFiles(*provider, nullptr);
            }
            else if (mode == ELoadingMode::AllButNew || mode == ELoadingMode::AllButModified)
            {
                // Both open a .fbkp backup and diff it against the mounted files; that needs the backup
                // reader (BackupManagerViewModel's format) and an LZ4 frame decoder.
                emit deferred(QStringLiteral("LoadingMode"), QStringLiteral("ViewModels/BackupManagerViewModel"));
                return;
            }
            else if (mode == ELoadingMode::AllButPatched)
            {
                token.throwIfCancellationRequested();
                entries = filterPatchedFiles(*provider);
            }

            status->updateStatusLabel(packageCountLabel(static_cast<int>(entries.size())));
            cue4Parse->assetsFolder()->bulkPopulate(entries);
        });

        // C# runs these after the tree is published, deliberately: for a streamed provider they may download
        // chunks and must not delay an otherwise valid tree.
        cue4Parse->loadVirtualPaths();
        cue4Parse->loadLocalizedResources();
    }
}
