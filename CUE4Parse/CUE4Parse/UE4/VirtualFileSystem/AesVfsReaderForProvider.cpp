// Ported from CUE4Parse/UE4/VirtualFileSystem/AesVfsReaderForProvider.cs
// C# reopens both reader classes as partials to add MountTo; here the declarations live on the class
// headers and only the definitions are grouped in this file, keeping the C# file split visible.
#include "AbstractAesVfsReader.h"
#include "AbstractVfsReader.h"

#include "../../FileProvider/Vfs/FileProviderDictionary.h"

namespace CUE4Parse::UE4::VirtualFileSystem
{
    void AbstractVfsReader::MountTo(FileProvider::Vfs::FileProviderDictionary& files,
                                    const Utils::StringComparer& pathComparer,
                                    const VfsEventHandler& vfsMounted)
    {
        Mount(pathComparer);

        files.AddFiles(Files(), ReadOrder());
        if (vfsMounted) vfsMounted(*this, files.Count());
    }

    void AbstractAesVfsReader::MountTo(FileProvider::Vfs::FileProviderDictionary& files,
                                       const Utils::StringComparer& pathComparer,
                                       std::shared_ptr<Encryption::Aes::FAesKey> key,
                                       const VfsEventHandler& vfsMounted)
    {
        AesKey() = std::move(key);
        AbstractVfsReader::MountTo(files, pathComparer, vfsMounted);
    }
}
