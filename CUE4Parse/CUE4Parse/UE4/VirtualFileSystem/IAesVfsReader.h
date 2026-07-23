// Ported from CUE4Parse/UE4/VirtualFileSystem/IAesVfsReader.cs
// The AES-capable slice of IVfsReader.
//
// C#'s `CustomEncryptionDelegate` (the per-game decryption hook) becomes a std::function with the same
// signature. None of the GameTypes implementations that populate it are ported, so in practice it stays
// null — but the seam is kept because the extract/index paths branch on it.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "IVfsReader.h"
#include "../../Compression/CompressionMethod.h"
#include "../../Encryption/Aes/FAesKey.h"
#include "../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::VirtualFileSystem
{
    class IAesVfsReader;

    // (bytes, beginOffset, count, isIndex, reader) -> decrypted bytes
    using CustomEncryptionDelegate = std::function<std::vector<uint8_t>(
        const std::vector<uint8_t>& bytes, int beginOffset, int count, bool isIndex, IAesVfsReader& reader)>;

    class IAesVfsReader : public virtual IVfsReader
    {
    public:
        virtual Objects::Core::Misc::FGuid EncryptionKeyGuid() const = 0;
        virtual int64_t Length() const = 0;

        virtual CustomEncryptionDelegate& CustomEncryption() = 0;
        virtual std::shared_ptr<Encryption::Aes::FAesKey>& AesKey() = 0;

        virtual std::vector<Compression::CompressionMethod>& CompressionMethods() = 0;
        virtual bool IsEncrypted() const = 0;
        virtual int EncryptedFileCount() const = 0;

        virtual bool TestAesKey(const Encryption::Aes::FAesKey& key) = 0;
        virtual std::vector<uint8_t> MountPointCheckBytes() = 0;
    };
}
