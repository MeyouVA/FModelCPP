// Ported from CUE4Parse/Encryption/Aes/Aes.cs
//
// C# gets AES from System.Security.Cryptography: an ECB provider with PaddingMode.None and a 128-bit block,
// used only ever to *decrypt*. C++ has no standard crypto, and the port does not vendor a crypto library, so
// the inverse cipher is implemented here directly (FIPS-197, 256-bit key, 14 rounds). This is the one place
// in the port where a BCL primitive had to be reimplemented rather than mapped, so it is covered by the
// published FIPS-197 / NIST SP 800-38A ECB-AES256 vectors in tests/test_aes.cpp.
//
// The C# file declares these as extension methods on byte[]/ArraySegment<byte>; C++ has no extension methods,
// so they are static members, and the ArraySegment overload collapses into the (data, offset, count) one.
//
// Encryption is deliberately not implemented: CUE4Parse never encrypts, and an unused encryptor would be
// untested code sitting next to the decryptor.
#pragma once

#include <cstdint>
#include <vector>

#include "FAesKey.h"

namespace CUE4Parse::Encryption::Aes
{
    class Aes
    {
    public:
        static constexpr int ALIGN = 16;
        // In *bits*, as in C# (it is fed to AesProvider.BlockSize, which is bit-valued). Not a byte count.
        static constexpr int BLOCK_SIZE = 16 * 8;

        // ECB, no padding. `encrypted.size()` must be a multiple of ALIGN, matching what
        // TransformFinalBlock accepts with PaddingMode.None.
        static std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& encrypted, const FAesKey& key);
        static std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& encrypted, int beginOffset, int count, const FAesKey& key);

        // No C# counterpart. C# always allocates a fresh array out of TransformFinalBlock; the extract path
        // here reads into a reusable buffer, and decrypting it in place saves a copy per compression block.
        static void DecryptInPlace(uint8_t* data, int count, const FAesKey& key);
    };
}
