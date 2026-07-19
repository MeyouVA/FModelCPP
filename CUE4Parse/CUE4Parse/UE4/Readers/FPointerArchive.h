// Ported from CUE4Parse/UE4/Readers/FPointerArchive.cs
// A random-access archive over a caller-owned raw byte pointer (no ownership / no copy).
// The pointed-to memory must outlive the archive.
#pragma once

#include <cstring>
#include <string>
#include <utility>

#include "FArchive.h"

namespace CUE4Parse::UE4::Readers
{
    class FPointerArchive : public FArchive
    {
    public:
        FPointerArchive(std::string name, const uint8_t* ptr, int64_t length, VersionContainer versions = VersionContainer())
            : FArchive(std::move(versions)), _ptr(ptr), _name(std::move(name))
        {
            Length = length;
        }

        int Read(uint8_t* buffer, int offset, int count) override;
        int ReadAt(int64_t position, uint8_t* buffer, int offset, int count) override;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        bool CanSeek() const override { return true; }
        const std::string& Name() const override { return _name; }
        std::unique_ptr<FArchive> Clone() const override;

        std::vector<uint8_t> ReadBytes(int length) override;
        void Serialize(uint8_t* ptr, int length) override;

        // Fast hiding overloads (see FArchive header note on non-virtual template specialization).
        template <typename T>
        T Read()
        {
            static_assert(std::is_trivially_copyable_v<T>, "FPointerArchive::Read<T> requires a trivially copyable T");
            T result{};
            std::memcpy(&result, _ptr + Position, sizeof(T));
            Position += static_cast<int64_t>(sizeof(T));
            return result;
        }

        template <typename T>
        std::vector<T> ReadArray(int length)
        {
            static_assert(std::is_trivially_copyable_v<T>, "FPointerArchive::ReadArray<T> requires a trivially copyable T");
            const int size = length * static_cast<int>(sizeof(T));
            std::vector<T> result(static_cast<size_t>(length));
            if (length > 0) std::memcpy(result.data(), _ptr + Position, static_cast<size_t>(size));
            Position += size;
            return result;
        }

        template <typename T>
        void ReadArray(std::vector<T>& array)
        {
            if (array.empty()) return;
            const int size = static_cast<int>(array.size()) * static_cast<int>(sizeof(T));
            std::memcpy(array.data(), _ptr + Position, static_cast<size_t>(size));
            Position += size;
        }

    private:
        const uint8_t* _ptr;
        std::string _name;
    };
}
