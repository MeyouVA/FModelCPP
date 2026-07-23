#include "AbstractAesVfsReader.h"

#include "../Exceptions/InvalidAesKeyException.h"
#include "../../Encryption/Aes/Aes.h"

namespace CUE4Parse::UE4::VirtualFileSystem
{
    using CUE4Parse::Encryption::Aes::Aes;

    bool AbstractAesVfsReader::TestAesKey(const std::vector<uint8_t>& bytes, const Encryption::Aes::FAesKey& key)
    {
        std::vector<uint8_t> result;
        if (_customEncryption)
        {
            // C# swaps AesKey in for the duration of the call and restores it afterwards, so the delegate
            // sees the candidate key rather than the reader's current one.
            auto backupKey = _aesKey;
            _aesKey = std::make_shared<Encryption::Aes::FAesKey>(key);
            try
            {
                result = _customEncryption(bytes, 0, static_cast<int>(bytes.size()), true, *this);
            }
            catch (...)
            {
                _aesKey = backupKey;
                throw;
            }
            _aesKey = backupKey;
        }
        else
        {
            result = Aes::Decrypt(bytes, key);
        }

        return IsValidIndex(result);
    }

    std::vector<uint8_t> AbstractAesVfsReader::Decrypt(const std::vector<uint8_t>& bytes,
                                                       const std::shared_ptr<Encryption::Aes::FAesKey>& key,
                                                       bool bypassMountPointCheck)
    {
        if (_bDecrypted)
        {
            if (!key) throw Exceptions::InvalidAesKeyException("Reading encrypted data requires a valid aes key");
            return Aes::Decrypt(bytes, *key);
        }

        if (key && (TestAesKey(*key) || bypassMountPointCheck))
        {
            _bDecrypted = true;
            return Aes::Decrypt(bytes, *key);
        }
        throw Exceptions::InvalidAesKeyException("Reading encrypted data requires a valid aes key");
    }

    std::vector<uint8_t> AbstractAesVfsReader::DecryptIfEncrypted(const std::vector<uint8_t>& bytes, bool isEncrypted, bool isIndex)
    {
        if (!isEncrypted) return bytes;
        if (_customEncryption)
            return _customEncryption(bytes, 0, static_cast<int>(bytes.size()), isIndex, *this);

        return Decrypt(bytes, _aesKey);
    }

    std::vector<uint8_t> AbstractAesVfsReader::DecryptIfEncrypted(const std::vector<uint8_t>& bytes, int beginOffset, int count,
                                                                  bool isEncrypted, bool bypassMountPointCheck, bool isIndex)
    {
        if (!isEncrypted) return bytes;
        if (_customEncryption)
            return _customEncryption(bytes, beginOffset, count, isIndex, *this);

        return Decrypt(bytes, _aesKey, bypassMountPointCheck);
    }

    std::vector<uint8_t> AbstractAesVfsReader::ReadAndDecryptAt(std::vector<uint8_t>& buffer, int64_t position, int length,
                                                                Readers::FArchive& reader, bool isEncrypted)
    {
        reader.ReadAt(position, buffer.data(), 0, length);
        return DecryptIfEncrypted(buffer, isEncrypted);
    }
}
