// Ported from CUE4Parse/UE4/Readers/FArchive.cs
// Abstract base for every reader in CUE4Parse. Provides the primitive read helpers (Read<T>, arrays,
// strings, packed ints, ...) on top of an abstract byte source implemented by subclasses.
//
// Differences from the C# version, by design:
//   * RandomAccessStream/Stream is not a dependency here; Position and Length are plain state on the
//     archive (this matches how FByteArchive already models them in the C# source).
//   * Generic methods become C++ templates. Because template methods cannot be virtual, the perf
//     specializations (e.g. FByteArchive::Read<T>) are provided as hiding overloads on the subclass.
//   * SerializeCompressedNew (compression layer) and ReadUObject (assets layer) are intentionally not
//     ported yet. TODO: bring them over with those layers.
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <span>
#include <memory>
#include <unordered_map>
#include <type_traits>

#include "../Versions/VersionContainer.h"
#include "../Versions/ObjectVersion.h"
#include "../Objects/UObject/FName.h"
#include "../Objects/Core/Misc/ECompressionFlags.h"
#include "../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Readers
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Assets::Exports::Texture::ETexturePlatform;

    // Mirrors System.IO.SeekOrigin.
    enum class ESeekOrigin { Begin, Current, End };

    class FArchive
    {
    public:
        VersionContainer Versions;

        // Position/Length are plain archive state (see header note).
        int64_t Position = 0;
        int64_t Length = 0;

        explicit FArchive(VersionContainer versions = VersionContainer()) : Versions(std::move(versions)) {}
        virtual ~FArchive() = default;

        // --- version accessors, proxied through Versions like the C# properties ---
        EGame Game() const { return Versions.Game(); }
        void SetGame(EGame value) { Versions.SetGame(value); }
        FPackageFileVersion Ver() const { return Versions.Ver(); }
        void SetVer(FPackageFileVersion value) { Versions.SetVer(value); }
        ETexturePlatform Platform() const { return Versions.Platform(); }

        virtual const std::string& Name() const = 0;

        bool SupportPartialReads() const
        {
            switch (Game())
            {
                case GAME_GameForPeace:
                case GAME_Rennsport:
                case GAME_DragonQuestXI:
                    return false;
                default:
                    return true;
            }
        }

        // --- abstract byte source ---
        // Read up to count bytes into buffer[offset..], advancing Position; returns the number read.
        virtual int Read(uint8_t* buffer, int offset, int count) = 0;
        virtual int64_t Seek(int64_t offset, ESeekOrigin origin) = 0;
        virtual bool CanSeek() const { return false; }
        virtual std::unique_ptr<FArchive> Clone() const = 0;

        virtual int ReadAt(int64_t position, uint8_t* buffer, int offset, int count)
        {
            Position = position;
            CheckReadSize(count);
            return Read(buffer, offset, count);
        }

        // --- byte reads ---
        virtual std::vector<uint8_t> ReadBytes(int length)
        {
            CheckReadSize(length);
            std::vector<uint8_t> result(static_cast<size_t>(length));
            if (length > 0) Read(result.data(), 0, length);
            return result;
        }

        std::vector<uint8_t> ReadBytesAt(int64_t position, int length)
        {
            std::vector<uint8_t> result(static_cast<size_t>(length));
            if (length > 0) ReadAt(position, result.data(), 0, length);
            return result;
        }

        // Copies exactly length bytes into ptr, advancing Position. Overridden by memory archives.
        virtual void Serialize(uint8_t* ptr, int length)
        {
            auto bytes = ReadBytes(length);
            if (length > 0) std::memcpy(ptr, bytes.data(), static_cast<size_t>(length));
        }

        // Reads a single value of `size` bytes into dst. This is the non-template seam that the
        // Read<T> template routes through, so endian-swapping archives (FArchiveBigEndian) can
        // affect Read<T> and every inherited helper that uses it — the role C#'s virtual Read<T> plays.
        virtual void ReadScalar(void* dst, int size)
        {
            Serialize(static_cast<uint8_t*>(dst), size);
        }

        // Reads `count` elements of `elementSize` bytes into dst. The seam ReadArray<T> routes through
        // (per-element endian swaps live here).
        virtual void ReadElements(void* dst, int elementSize, int count)
        {
            if (count > 0) Serialize(static_cast<uint8_t*>(dst), elementSize * count);
        }

        // Returns a view over the next length bytes (valid until the next read). See header note on lifetime.
        virtual std::span<const uint8_t> ReadSpan(int length)
        {
            CheckReadSize(length);
            _spanScratch.resize(static_cast<size_t>(length));
            if (length > 0) Read(_spanScratch.data(), 0, length);
            return std::span<const uint8_t>(_spanScratch.data(), static_cast<size_t>(length));
        }

        // --- typed reads (templates; may be hidden by faster subclass overloads) ---
        template <typename T>
        T Read()
        {
            static_assert(std::is_trivially_copyable_v<T>, "FArchive::Read<T> requires a trivially copyable T");
            T result{};
            ReadScalar(&result, static_cast<int>(sizeof(T)));
            return result;
        }

        template <typename T>
        std::vector<T> ReadArray(int length)
        {
            static_assert(std::is_trivially_copyable_v<T>, "FArchive::ReadArray<T> requires a trivially copyable T");
            const int readLength = static_cast<int>(sizeof(T)) * length;
            CheckReadSize(readLength);
            std::vector<T> result(static_cast<size_t>(length));
            ReadElements(result.data(), static_cast<int>(sizeof(T)), length);
            return result;
        }

        template <typename T>
        void ReadArray(std::vector<T>& array)
        {
            if (array.empty()) return;
            const int readLength = static_cast<int>(sizeof(T)) * static_cast<int>(array.size());
            CheckReadSize(readLength);
            ReadElements(array.data(), static_cast<int>(sizeof(T)), static_cast<int>(array.size()));
        }

        // ReadArray with a per-element getter (callable returning T).
        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadArrayWith(int length, Getter getter)
        {
            std::vector<T> result;
            result.reserve(static_cast<size_t>(length));
            for (int i = 0; i < length; i++) result.push_back(getter());
            return result;
        }

        // Count-prefixed array with a getter (reads an int length, then that many elements).
        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadArrayWith(Getter getter)
        {
            const int length = Read<int32_t>();
            return ReadArrayWith<Getter, T>(length, getter);
        }

        // Count-prefixed POD array (reads an int length, then that many trivially-copyable elements).
        template <typename T>
        std::vector<T> ReadArrayCounted()
        {
            const int length = Read<int32_t>();
            return length > 0 ? ReadArray<T>(length) : std::vector<T>{};
        }

        // --- bulk arrays (UE "BulkSerialize" with an element-size sanity check) ---
        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadBulkArray(int elementSize, int elementCount, Getter getter)
        {
            const int64_t pos = Position;
            std::vector<T> array = ReadArrayWith<Getter, T>(elementCount, getter);
            if (Game() != GAME_HogwartsLegacy && !array.empty() &&
                Position != pos + static_cast<int64_t>(array.size()) * elementSize)
            {
                throw Exceptions::ParserException(
                    "RawArray item size mismatch: expected " + std::to_string(elementSize) +
                    ", serialized " + std::to_string((Position - pos) / static_cast<int64_t>(array.size())));
            }
            return array;
        }

        template <typename T>
        std::vector<T> ReadBulkArray()
        {
            if (Ver() < EUnrealEngineObjectUE3Version::ADDED_BULKSERIALIZE_SANITY_CHECKING)
                return ReadArrayCounted<T>();

            const int elementSize = Read<int32_t>();
            const int elementCount = Read<int32_t>();
            if (elementCount == 0) return {};

            const int64_t pos = Position;
            std::vector<T> array = ReadArray<T>(elementCount);
            if (Position != pos + static_cast<int64_t>(array.size()) * elementSize)
            {
                throw Exceptions::ParserException(
                    "RawArray item size mismatch: expected " + std::to_string(elementSize) +
                    ", serialized " + std::to_string((Position - pos) / static_cast<int64_t>(array.size())));
            }
            return array;
        }

        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadBulkArray(Getter getter)
        {
            if (Ver() < EUnrealEngineObjectUE3Version::ADDED_BULKSERIALIZE_SANITY_CHECKING)
                return ReadArrayWith<Getter, T>(getter);

            const int elementSize = Read<int32_t>();
            const int elementCount = Read<int32_t>();
            return ReadBulkArray<Getter, T>(elementSize, elementCount, getter);
        }

        void SkipBulkArrayData()
        {
            if (Ver() < EUnrealEngineObjectUE3Version::ADDED_BULKSERIALIZE_SANITY_CHECKING)
                throw Exceptions::ParserException("Cannot skip bulk array data for UE3 versions before ADDED_BULKSERIALIZE_SANITY_CHECKING");
            const int elementSize = Read<int32_t>();
            const int elementCount = Read<int32_t>();
            Position += static_cast<int64_t>(elementSize) * elementCount;
        }

        void SkipMultipleBulkArrayData(int count) { for (int i = 0; i < count; i++) SkipBulkArrayData(); }

        void SkipFixedArray(int size = -1)
        {
            const int num = Read<int32_t>();
            Position += static_cast<int64_t>(num) * size;
        }

        void SkipMultipleFixedArrays(const std::vector<int>& sizes) { for (int s : sizes) SkipFixedArray(s); }
        void SkipMultipleFixedArrays(int count, int size) { for (int i = 0; i < count; i++) SkipFixedArray(size); }

        // --- maps ---
        template <typename TKey, typename TValue, typename KeyGetter, typename ValueGetter>
        std::unordered_map<TKey, TValue> ReadMap(int length, KeyGetter keyGetter, ValueGetter valueGetter)
        {
            std::unordered_map<TKey, TValue> res;
            res.reserve(static_cast<size_t>(length));
            for (int i = 0; i < length; i++)
            {
                TKey key = keyGetter();
                res[key] = valueGetter();
            }
            return res;
        }

        template <typename TKey, typename TValue, typename KeyGetter, typename ValueGetter>
        std::unordered_map<TKey, TValue> ReadMap(KeyGetter keyGetter, ValueGetter valueGetter)
        {
            const int length = Read<int32_t>();
            return ReadMap<TKey, TValue>(length, keyGetter, valueGetter);
        }

        template <typename TKey, typename TValue, typename KeyGetter, typename ValueGetter>
        std::unordered_map<TKey, std::vector<TValue>> ReadMultiMap(int length, KeyGetter keyGetter, ValueGetter valueGetter)
        {
            std::unordered_map<TKey, std::vector<TValue>> result;
            for (int i = 0; i < length; i++)
            {
                TKey key = keyGetter();
                TValue value = valueGetter();
                result[key].push_back(value);
            }
            return result;
        }

        template <typename TKey, typename TValue, typename KeyGetter, typename ValueGetter>
        std::unordered_map<TKey, std::vector<TValue>> ReadMultiMap(KeyGetter keyGetter, ValueGetter valueGetter)
        {
            const int length = Read<int32_t>();
            return ReadMultiMap<TKey, TValue>(length, keyGetter, valueGetter);
        }

        // --- scalar / string reads (defined in FArchive.cpp) ---
        virtual bool ReadBoolean();
        bool ReadFlag();
        virtual uint32_t ReadIntPacked();
        int Read7BitEncodedInt();
        std::string ReadString();
        void SkipFString();
        virtual std::string ReadFString();
        std::string ReadFUtf8String();
        std::string ReadFUtf8String(int length);
        std::string ReadFAnsiString();
        virtual std::string ReadFAnsiString(int length);
        float ReadFReal();
        virtual FName ReadFName();

        // Bulk-data compressed-header layout (mirrors C#'s nested FCompressedChunkInfo). Two sizes,
        // read as uint32 for pre-UE4 games and int64 otherwise. Note the C# quirk that a raw
        // Read<FCompressedChunkInfo>() blits 16 bytes (two int64s) regardless of game — preserved here.
        struct FCompressedChunkInfo
        {
            int64_t CompressedSize = 0;
            int64_t UncompressedSize = 0;

            FCompressedChunkInfo() = default;
            explicit FCompressedChunkInfo(FArchive& Ar)
            {
                CompressedSize = Ar.Game() < GAME_UE4_0 ? static_cast<int64_t>(Ar.Read<uint32_t>()) : Ar.Read<int64_t>();
                UncompressedSize = Ar.Game() < GAME_UE4_0 ? static_cast<int64_t>(Ar.Read<uint32_t>()) : Ar.Read<int64_t>();
            }
        };

        // Reads a "SerializeCompressedNew" bulk-data blob into dest, decompressing each chunk. Mirrors
        // FArchive.SerializeCompressedNew; used by FArchiveLoadCompressedProxy.
        void SerializeCompressedNew(uint8_t* dest, int length, const std::string& compressionFormatToDecodeOldV1Files,
                                    Objects::Core::Misc::ECompressionFlags flags, bool bTreatBufferAsFileReader,
                                    int64_t& outPartialReadLength);

        void CheckReadSize(int length);

    protected:
        // Reusable backing store for ReadSpan (see header note on lifetime).
        std::vector<uint8_t> _spanScratch;
    };
}
