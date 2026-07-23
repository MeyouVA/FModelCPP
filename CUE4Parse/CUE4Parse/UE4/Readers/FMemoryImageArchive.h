// Ported from CUE4Parse/UE4/Readers/FMemoryImageArchive.cs
//
// The reader for UE's "frozen" / memory-image layout: structures are serialised as their in-memory image, so
// arrays and strings are not inline but are reached through a relative pointer stored where the C++ pointer
// field sat. Every container read here is the same shape — read an FFrozenMemoryImagePtr, read Num and Max,
// jump to (ptrPos + OffsetFromThis), read the payload, jump back.
//
// Deliberate differences from C#:
//   * C# overrides the Position property to forward to InnerArchive. Position is a plain field on the C++
//     FArchive, so instead every byte-source seam (Read/Seek/Serialize/ReadScalar/ReadElements/ReadBytes/
//     ReadSpan) syncs Position into the inner archive and reads it back out. Because all the templated
//     helpers on FArchive route through those seams, that keeps the two positions in lockstep without
//     needing a virtual property.
//   * InnerArchive is a shared_ptr, not a bare reference: the frozen-index path builds a throwaway
//     FByteArchive purely to feed this reader, so the reader has to be able to own it.
//   * C#'s ReadArray<T>() (count-prefixed) is spelled ReadArrayCounted<T>() here, matching the names the
//     C++ FArchive already uses; ReadArray<T>(Func<T>) becomes ReadArrayWith(getter).
//   * BitArray becomes std::vector<bool>; the TMap/TSet readers return a vector of pairs rather than an
//     IEnumerable of tuples.
//   * ReadMaterialParameterType / ReadMaterialUniformPreshaderHeader and the PointerTable field are NOT
//     ported: EMaterialParameterType, FMaterialUniformPreshaderHeader and FPointerTableBase all belong to
//     the material/UObject layers that have no C++ counterpart yet. Nothing on the pak path touches them.
//     TODO: bring them over with the material layer.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "FArchive.h"
#include "../Exceptions/ParserException.h"
#include "../Objects/UObject/FName.h"
#include "../Versions/EGame.h"
#include "../../Utils/AlignUtils.h"
#include "../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Readers
{
    class FMemoryImageArchive;

    // A frozen pointer: 64 bits packing "is this frozen", a byte offset relative to the pointer's own
    // position, and (UE5+) an index into the type table.
    struct FFrozenMemoryImagePtr
    {
        static constexpr uint64_t TypeIndexMask = ((1ULL << 23) - 1ULL) << 1;

        uint64_t Packed = 0;
        bool IsFrozen = false;
        int64_t OffsetFromThis = 0;
        int32_t TypeIndex = -1;

        explicit FFrozenMemoryImagePtr(FMemoryImageArchive& Ar);
    };

    class FMemoryImageArchive : public FArchive
    {
    public:
        std::shared_ptr<FArchive> InnerArchive;

        // C#'s `IReadOnlyDictionary<int, (FName, bool)>? Names`. Non-owning: the shader-map reader that
        // populates it outlives the archive. The bool is bIsScriptName.
        const std::unordered_map<int32_t, std::pair<Objects::UObject::FName, bool>>* Names = nullptr;

        explicit FMemoryImageArchive(std::shared_ptr<FArchive> ar);
        FMemoryImageArchive(std::shared_ptr<FArchive> ar, int arrayAlign);

        // --- byte-source seams, each syncing Position with the inner archive ---
        using FArchive::Read; // keep the inherited Read<T> template visible alongside the byte-source override

        int Read(uint8_t* buffer, int offset, int count) override;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        bool CanSeek() const override { return InnerArchive->CanSeek(); }
        const std::string& Name() const override { return InnerArchive->Name(); }
        std::unique_ptr<FArchive> Clone() const override;

        std::vector<uint8_t> ReadBytes(int length) override;
        void Serialize(uint8_t* ptr, int length) override;
        void ReadScalar(void* dst, int size) override;
        void ReadElements(void* dst, int elementSize, int count) override;
        std::span<const uint8_t> ReadSpan(int length) override;

        std::string ReadFString() override;
        Objects::UObject::FName ReadFName() override;

        // --- frozen containers ---

        // C#'s ReadArray<T>(): a frozen TArray of trivially-copyable elements.
        template <typename T>
        std::vector<T> ReadArrayCounted()
        {
            int32_t arrayNum = 0;
            const int64_t continuePos = BeginContainer(arrayNum);
            if (arrayNum == 0) return {};

            std::vector<T> data = InnerReadArray<T>(arrayNum);
            Position = continuePos;
            return data;
        }

        // C#'s ReadArray<T>(Func<T>). realignAfterElement=false is C#'s two-argument overload.
        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadArrayWith(Getter getter, bool realignAfterElement = true)
        {
            int32_t arrayNum = 0;
            const int64_t continuePos = BeginContainer(arrayNum);
            if (arrayNum == 0) return {};

            std::vector<T> data;
            data.reserve(static_cast<size_t>(arrayNum));
            for (int32_t i = 0; i < arrayNum; ++i)
            {
                data.push_back(getter());
                if (realignAfterElement) Position = Utils::Align(Position, _arrayAlign);
            }
            Position = continuePos;
            return data;
        }

        // A frozen array of pointers to elements, each element reached through its own frozen pointer.
        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadArrayOfPtrs(Getter getter)
        {
            int32_t arrayNum = 0;
            const int64_t continuePos = BeginContainer(arrayNum);
            if (arrayNum == 0) return {};

            std::vector<T> data;
            data.reserve(static_cast<size_t>(arrayNum));
            for (int32_t i = 0; i < arrayNum; ++i)
            {
                const int64_t entryPtrPos = Position;
                const FFrozenMemoryImagePtr entryPtr(*this);
                Position = entryPtrPos + entryPtr.OffsetFromThis;
                data.push_back(getter());
                Position = Utils::Align(entryPtrPos + 8, 8);
            }
            Position = continuePos;
            return data;
        }

        // Consumes the header of a frozen hash table. C# returns an empty array unconditionally (the
        // contents are never needed — the elements are reachable through the sparse array); kept as-is.
        std::vector<int32_t> ReadHashTable();

        std::vector<bool> ReadTBitArray();

        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadTSparseArray(Getter elementGetter, int elementStructSize)
        {
            const int64_t initialPos = Position;
            const FFrozenMemoryImagePtr dataPtr(*this);
            const int32_t dataNum = Read<int32_t>();
            (void) Read<int32_t>(); // dataMax
            const std::vector<bool> allocationFlags = ReadTBitArray();
            Position += 4 + 4; // skip FirstFreeIndex and NumFreeIndices
            if (dataNum == 0) return {};

            const int64_t continuePos = Position;
            Position = initialPos + dataPtr.OffsetFromThis;
            std::vector<T> data;
            data.reserve(static_cast<size_t>(dataNum));
            for (int32_t i = 0; i < dataNum; ++i)
            {
                const int64_t start = Position;
                if (static_cast<size_t>(i) < allocationFlags.size() && allocationFlags[static_cast<size_t>(i)])
                    data.push_back(elementGetter());
                Position = start + elementStructSize;
            }

            Position = continuePos;
            return data;
        }

        template <typename Getter, typename T = std::invoke_result_t<Getter&>>
        std::vector<T> ReadTSet(Getter elementGetter, int elementStructSize)
        {
            // + HashNextId and HashIndex from TSetElement
            std::vector<T> elements = ReadTSparseArray(elementGetter, elementStructSize + 4 + 4);
            Position += 8 + 4; // skip Hash and HashSize
            return elements;
        }

        template <typename KeyGetter, typename ValueGetter,
                  typename TKey = std::invoke_result_t<KeyGetter&>,
                  typename TValue = std::invoke_result_t<ValueGetter&>>
        std::vector<std::pair<TKey, TValue>> ReadTMap(KeyGetter keyGetter, ValueGetter valueGetter,
                                                      int keyStructSize, int valueStructSize)
        {
            // The key is read before the value, so the pair getter must not rely on argument evaluation order.
            auto pairGetter = [&]() -> std::pair<TKey, TValue>
            {
                TKey key = keyGetter();
                TValue value = valueGetter();
                return {std::move(key), std::move(value)};
            };
            return ReadTSet(pairGetter, static_cast<int>(Utils::Align(keyStructSize + valueStructSize, 8)));
        }

    private:
        // Reads the frozen ptr + Num/Max header shared by every container, validates Num == Max, and seeks
        // to the payload. Returns the position to restore afterwards; leaves Position at the payload.
        int64_t BeginContainer(int32_t& arrayNum);

        // ReadArray<T> straight off the inner archive (bypassing this archive's seams), as C# does.
        template <typename T>
        std::vector<T> InnerReadArray(int32_t count)
        {
            InnerArchive->Position = Position;
            std::vector<T> data = InnerArchive->ReadArray<T>(count);
            Position = InnerArchive->Position;
            return data;
        }

        int _arrayAlign = 4;
    };
}
