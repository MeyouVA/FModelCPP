// Ported from FModel/ViewModels/GameDirectoryViewModel.cs
#include "GameDirectoryViewModel.h"

#include <algorithm>
#include <regex>

#include "UE4/IO/IoStoreReader.h"
#include "UE4/VirtualFileSystem/IAesVfsReader.h"

namespace FModel::ViewModels
{
    using CUE4Parse::UE4::IO::IoStoreReader;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader;

    namespace
    {
        // C#'s [GeneratedRegex(@"^(?!global|pakchunk.+(optional|ondemand)\-).+(pak|utoc)$", IgnoreCase |
        // Compiled | Singleline | CultureInvariant)]. ECMAScript std::regex supports the negative lookahead.
        const std::regex& archivesRegex()
        {
            static const std::regex re(R"(^(?!global|pakchunk.+(optional|ondemand)\-).+(pak|utoc)$)",
                                       std::regex::ECMAScript | std::regex::icase);
            return re;
        }

        // SortDescription(IsLooseFilesContainer, Ascending) then SortDescription(Name, Ascending): false
        // sorts before true, so the real archives come first and "Loose Files" lands at the bottom.
        bool byContainerThenName(FileItem* a, FileItem* b)
        {
            if (a->isLooseFilesContainer() != b->isLooseFilesContainer())
                return !a->isLooseFilesContainer();
            return a->name() < b->name();
        }
    }

    FileItem::FileItem(const QString& name, int64_t length, QObject* parent)
        : ViewModel(parent), _name(name), _length(length)
    {
    }

    FileItem::FileItem(const QString& name, int fileCount, int64_t length, bool isLooseFile, QObject* parent)
        : ViewModel(parent), _name(name), _length(length), _fileCount(fileCount), _isEnabled(true),
          _isLooseFilesContainer(isLooseFile)
    {
        // C# assigns Key = string.Empty, MountPoint = string.Empty, CompressionMethods = [] — already the
        // default-constructed state of each member here.
    }

    FileItem::FileItem(IAesVfsReader& reader, QObject* parent)
        : ViewModel(parent),
          _name(QString::fromStdString(reader.Name())),
          _length(reader.Length()),
          _isEncrypted(reader.IsEncrypted()),
          _isEnabled(false),
          _isLooseFilesContainer(false),
          _guid(reader.EncryptionKeyGuid()),
          _compressionMethods(reader.CompressionMethods())
    {
        // An IO Store container knows its entry count before it is mounted; a pak does not, and reports 0
        // until Verify fills it in. The `- 1` is upstream's: the TOC counts itself.
        if (auto* storeReader = dynamic_cast<IoStoreReader*>(&reader); storeReader != nullptr &&
            storeReader->TocResource.Header != nullptr)
            _fileCount = static_cast<int>(storeReader->TocResource.Header->TocEntryCount) - 1;
    }

    void FileItem::setFileCount(int value) { setProperty(_fileCount, value, QStringLiteral("FileCount")); }
    void FileItem::setMountPoint(const QString& value) { setProperty(_mountPoint, value, QStringLiteral("MountPoint")); }
    void FileItem::setIsEncrypted(bool value) { setProperty(_isEncrypted, value, QStringLiteral("IsEncrypted")); }
    void FileItem::setIsEnabled(bool value) { setProperty(_isEnabled, value, QStringLiteral("IsEnabled")); }
    void FileItem::setIsLooseFilesContainer(bool value)
    { setProperty(_isLooseFilesContainer, value, QStringLiteral("IsLooseFilesContainer")); }
    void FileItem::setKey(const QString& value) { setProperty(_key, value, QStringLiteral("Key")); }

    void FileItem::setGuid(const FGuid& value)
    {
        if (_guid == value)
            return;
        _guid = value;
        raisePropertyChanged(QStringLiteral("Guid"));
    }

    QString FileItem::toString() const
    {
        return QStringLiteral("%1 | %2").arg(_name, _key);
    }

    GameDirectoryViewModel::GameDirectoryViewModel(QObject* parent)
        : ViewModel(parent), _directoryFiles(this), _directoryFilesView(&_directoryFiles, byContainerThenName)
    {
    }

    bool GameDirectoryViewModel::isVisibleArchive(const QString& name)
    {
        const std::string s = name.toStdString();
        return std::regex_match(s, archivesRegex());
    }

    void GameDirectoryViewModel::add(IAesVfsReader& reader)
    {
        // C#: `if (!_hiddenArchives.IsMatch(reader.Name)) return;` — despite the field name, a MATCH means
        // the archive IS shown; the negative lookahead is what hides global/optional/on-demand containers.
        if (!isVisibleArchive(QString::fromStdString(reader.Name())))
            return;

        _directoryFiles.add(new FileItem(reader, this));
    }

    void GameDirectoryViewModel::addLooseFiles(int fileCount)
    {
        if (fileCount < 1)
            return;

        for (FileItem* item : _directoryFiles.items())
        {
            if (item->isLooseFilesContainer())
            {
                // A second call adds to the existing row rather than making another one.
                item->setFileCount(item->fileCount() + fileCount);
                return;
            }
        }

        _directoryFiles.add(new FileItem(QStringLiteral("Loose Files"), fileCount, 0, true, this));
    }

    void GameDirectoryViewModel::verify(IAesVfsReader& reader)
    {
        const QString name = QString::fromStdString(reader.Name());
        for (FileItem* item : _directoryFiles.items())
        {
            if (item->name() != name)
                continue;

            item->setIsEnabled(true);
            item->setMountPoint(QString::fromStdString(reader.MountPoint()));
            // C# reads `reader.FileCount`, which IVfsReader does not expose here; its file map is the same
            // number by definition (C#'s FileCount is `Files.Count`).
            item->setFileCount(static_cast<int>(reader.Files().size()));
            return;
        }
    }

    void GameDirectoryViewModel::disable(IAesVfsReader& reader)
    {
        const QString name = QString::fromStdString(reader.Name());
        for (FileItem* item : _directoryFiles.items())
        {
            if (item->name() != name)
                continue;
            item->setIsEnabled(false);
            return;
        }
    }
}
