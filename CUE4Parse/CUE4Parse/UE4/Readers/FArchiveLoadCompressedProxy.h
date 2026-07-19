// Ported from CUE4Parse/UE4/Readers/FArchiveLoadCompressedProxy.cs
// A forward-only archive that decompresses a "SerializeCompressedNew" blob on demand: reads pull from
// a decompressed scratch buffer that is refilled a chunk at a time via FArchive::SerializeCompressedNew.
//
// Deliberate difference: C# makes the Length property throw InvalidOperationException. Here Length is a
// plain field on FArchive used by CheckReadSize, so it is set to INT64_MAX (the decompressed size is not
// known ahead of time) — the base read helpers then never spuriously fail on it. Position tracks the
// number of raw (decompressed) bytes served, matching the C# Position getter.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "FArchive.h"
#include "../Objects/Core/Misc/ECompressionFlags.h"

namespace CUE4Parse::UE4::Readers
{
    class FArchiveLoadCompressedProxy : public FArchive
    {
    public:
        FArchiveLoadCompressedProxy(std::string name, std::vector<uint8_t> compressedData, std::string compressionFormat,
                                    Objects::Core::Misc::ECompressionFlags flags = Objects::Core::Misc::COMPRESS_None,
                                    VersionContainer versions = VersionContainer());

        int Read(uint8_t* dstData, int offset, int count) override;
        // Keep the inherited Read<T>/ReadArray<T> templates visible (the byte-source Read override
        // above would otherwise hide them by name).
        using FArchive::Read;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        bool CanSeek() const override { return true; }
        const std::string& Name() const override { return _name; }
        std::unique_ptr<FArchive> Clone() const override;

    private:
        void DecompressMoreData();

        std::string _name;
        std::vector<uint8_t> _compressedData;
        int _currentIndex = 0;
        std::vector<uint8_t> _tmpData;
        int _tmpDataPos = 0;
        int _tmpDataSize = 0;
        bool _shouldSerializeFromArray = false;
        int64_t _rawBytesSerialized = 0;
        std::string _compressionFormat;
        Objects::Core::Misc::ECompressionFlags _compressionFlags;
    };
}
