// Ported from CUE4Parse/UE4/VirtualFileSystem/AbstractAesVfsReader.cs
// Adds the decrypt-if-encrypted plumbing on top of AbstractVfsReader: every read that might be encrypted
// goes through one of the ReadAndDecrypt* helpers, which consult CustomEncryption first and fall back to
// plain AES-256-ECB.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "AbstractVfsReader.h"
#include "IAesVfsReader.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::VirtualFileSystem
{
    class AbstractAesVfsReader : public AbstractVfsReader, public IAesVfsReader
    {
    public:
        CustomEncryptionDelegate& CustomEncryption() override { return _customEncryption; }
        std::shared_ptr<Encryption::Aes::FAesKey>& AesKey() override { return _aesKey; }
        std::vector<Compression::CompressionMethod>& CompressionMethods() override { return _compressionMethods; }

        int EncryptedFileCount() const override { return _encryptedFileCount; }
        bool Decrypted() const { return _bDecrypted; }

        bool TestAesKey(const Encryption::Aes::FAesKey& key) override
        {
            return !IsEncrypted() || TestAesKey(MountPointCheckBytes(), key);
        }

    protected:
        AbstractAesVfsReader(std::string path, Versions::VersionContainer versions)
            : AbstractVfsReader(std::move(path), std::move(versions)) {}

        bool TestAesKey(const std::vector<uint8_t>& bytes, const Encryption::Aes::FAesKey& key);

        // Throws InvalidAesKeyException when the key is missing or does not pass the mount-point probe.
        std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& bytes,
                                     const std::shared_ptr<Encryption::Aes::FAesKey>& key,
                                     bool bypassMountPointCheck = false);

        std::vector<uint8_t> DecryptIfEncrypted(const std::vector<uint8_t>& bytes)
        { return DecryptIfEncrypted(bytes, IsEncrypted()); }
        std::vector<uint8_t> DecryptIfEncrypted(const std::vector<uint8_t>& bytes, int beginOffset, int count)
        { return DecryptIfEncrypted(bytes, beginOffset, count, IsEncrypted()); }
        std::vector<uint8_t> DecryptIfEncrypted(const std::vector<uint8_t>& bytes, bool isEncrypted, bool isIndex = false);
        std::vector<uint8_t> DecryptIfEncrypted(const std::vector<uint8_t>& bytes, int beginOffset, int count,
                                                bool isEncrypted, bool bypassMountPointCheck = false, bool isIndex = false);

        virtual std::vector<uint8_t> ReadAndDecrypt(int length) = 0;
        virtual std::vector<uint8_t> ReadAndDecryptIndex(int length) { return ReadAndDecrypt(length); }

        std::vector<uint8_t> ReadAndDecrypt(int length, Readers::FArchive& reader, bool isEncrypted)
        { return DecryptIfEncrypted(reader.ReadBytes(length), isEncrypted); }

        std::vector<uint8_t> ReadAndDecryptAt(int64_t position, int length, Readers::FArchive& reader, bool isEncrypted)
        { return DecryptIfEncrypted(reader.ReadBytesAt(position, length), isEncrypted); }

        // Reads into the caller's reusable buffer, then decrypts it — the hot path in Extract.
        std::vector<uint8_t> ReadAndDecryptAt(std::vector<uint8_t>& buffer, int64_t position, int length,
                                              Readers::FArchive& reader, bool isEncrypted);

        std::vector<uint8_t> ReadAndDecryptIndex(int length, Readers::FArchive& reader, bool isEncrypted)
        { return DecryptIfEncrypted(reader.ReadBytes(length), isEncrypted, true); }

        std::vector<uint8_t> ReadAndDecryptIndexAt(int64_t position, int length, Readers::FArchive& reader, bool isEncrypted)
        { return DecryptIfEncrypted(reader.ReadBytesAt(position, length), isEncrypted, true); }

        int _encryptedFileCount = 0;
        bool _bDecrypted = false;

    private:
        CustomEncryptionDelegate _customEncryption;
        std::shared_ptr<Encryption::Aes::FAesKey> _aesKey;
        std::vector<Compression::CompressionMethod> _compressionMethods;
    };
}
