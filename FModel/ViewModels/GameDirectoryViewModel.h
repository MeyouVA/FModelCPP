#pragma once
// Ported from FModel/ViewModels/GameDirectoryViewModel.cs — the Archives tab: one FileItem per mountable
// container, plus a synthetic row for the loose files on disk.
//
// FileItem does double duty upstream: it is both an archive row here AND an AES-key row in the AES manager
// (which is why it carries a Key and a Guid that mean nothing to this list). That is kept.
//
// Deliberate differences from C#:
//   * `Application.Current.Dispatcher.Invoke` around each mutation is dropped — the port's load path is
//     synchronous (see ThreadWorkerViewModel.h), so there is no worker thread to marshal off.
//   * The `[GeneratedRegex]` source generator becomes a std::regex built once. The pattern is verbatim,
//     including its negative lookahead, and it is applied to the reader NAME (so `global.utoc` and the
//     optional/on-demand pakchunks never appear in the list).
//   * ICollectionView becomes Framework::CollectionView, with the two SortDescriptions
//     (IsLooseFilesContainer ascending, then Name ascending) expressed as one comparison.

#include <cstdint>
#include <vector>

#include <QObject>
#include <QString>

#include "Compression/CompressionMethod.h"
#include "UE4/Objects/Core/Misc/FGuid.h"

#include "../Framework/CollectionView.h"
#include "../Framework/RangeObservableCollection.h"
#include "../Framework/ViewModel.h"

namespace CUE4Parse::UE4::VirtualFileSystem { class IAesVfsReader; }

namespace FModel::ViewModels
{
    class FileItem : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        // C#: FileItem(string name, long length) — the AES manager's synthetic "Main Static Key" row.
        FileItem(const QString& name, int64_t length, QObject* parent = nullptr);
        // C#: FileItem(string name, int fileCount, long length, bool isLooseFile)
        FileItem(const QString& name, int fileCount, int64_t length, bool isLooseFile, QObject* parent = nullptr);
        // C#: FileItem(IAesVfsReader reader)
        explicit FileItem(CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader& reader, QObject* parent = nullptr);

        const QString& name() const { return _name; }
        int64_t length() const { return _length; }

        int fileCount() const { return _fileCount; }
        void setFileCount(int value);

        const QString& mountPoint() const { return _mountPoint; }
        void setMountPoint(const QString& value);

        bool isEncrypted() const { return _isEncrypted; }
        void setIsEncrypted(bool value);

        bool isEnabled() const { return _isEnabled; }
        void setIsEnabled(bool value);

        bool isLooseFilesContainer() const { return _isLooseFilesContainer; }
        void setIsLooseFilesContainer(bool value);

        const QString& key() const { return _key; }
        void setKey(const QString& value);

        const CUE4Parse::UE4::Objects::Core::Misc::FGuid& guid() const { return _guid; }
        void setGuid(const CUE4Parse::UE4::Objects::Core::Misc::FGuid& value);

        const std::vector<CUE4Parse::Compression::CompressionMethod>& compressionMethods() const
        { return _compressionMethods; }

        // C#: $"{Name} | {Key}"
        QString toString() const;

    private:
        QString _name;
        int64_t _length = 0;
        int _fileCount = 0;
        QString _mountPoint;
        bool _isEncrypted = false;
        bool _isEnabled = false;
        bool _isLooseFilesContainer = false;
        QString _key;
        CUE4Parse::UE4::Objects::Core::Misc::FGuid _guid;
        std::vector<CUE4Parse::Compression::CompressionMethod> _compressionMethods;
    };

    class GameDirectoryViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit GameDirectoryViewModel(QObject* parent = nullptr);

        Framework::RangeObservableCollection<FileItem*>& directoryFiles() { return _directoryFiles; }
        const Framework::RangeObservableCollection<FileItem*>& directoryFiles() const { return _directoryFiles; }

        Framework::CollectionView<FileItem*>& directoryFilesView() { return _directoryFilesView; }

        // Called from the provider's VfsRegistered event. Rows whose name the regex rejects never appear.
        void add(CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader& reader);
        void addLooseFiles(int fileCount);
        // VfsMounted: the archive opened, so it becomes selectable and reports its real contents.
        void verify(CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader& reader);
        // VfsUnmounted.
        void disable(CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader& reader);

        // C#'s [GeneratedRegex] — exposed so a test can pin which archive names the list hides.
        static bool isVisibleArchive(const QString& name);

    private:
        Framework::RangeObservableCollection<FileItem*> _directoryFiles;
        Framework::CollectionView<FileItem*> _directoryFilesView;
    };
}
