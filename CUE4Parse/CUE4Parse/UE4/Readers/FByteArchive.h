// Ported from CUE4Parse/UE4/Readers/FByteArchive.cs
// A random-access archive backed by an in-memory byte buffer.
#pragma once

#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "FArchive.h"

namespace CUE4Parse::UE4::Readers
{
    class FByteArchive : public FArchive
    {
    public:
        FByteArchive(std::string name, std::vector<uint8_t> data, VersionContainer versions = VersionContainer())
            : FArchive(std::move(versions)), _data(std::move(data)), _name(std::move(name))
        {
            Length = static_cast<int64_t>(_data.size());
        }

        FByteArchive(std::string name, std::vector<uint8_t> data, int64_t length, VersionContainer versions = VersionContainer())
            : FArchive(std::move(versions)), _data(std::move(data)), _name(std::move(name))
        {
            Length = length;
        }

        int Read(uint8_t* buffer, int offset, int count) override;
        int ReadAt(int64_t position, uint8_t* buffer, int offset, int count) override;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        bool CanSeek() const override { return true; }
        const std::string& Name() const override { return _name; }
        std::unique_ptr<FArchive> Clone() const override;

        std::span<const uint8_t> ReadSpan(int length) override;
        void Serialize(uint8_t* ptr, int length) override;

        // Fast hiding overloads (see FArchive header note on non-virtual template specialization).
        template <typename T>
        T Read()
        {
            static_assert(std::is_trivially_copyable_v<T>, "FByteArchive::Read<T> requires a trivially copyable T");
            T result{};
            std::memcpy(&result, _data.data() + Position, sizeof(T));
            Position += static_cast<int64_t>(sizeof(T));
            return result;
        }

        template <typename T>
        std::vector<T> ReadArray(int length)
        {
            static_assert(std::is_trivially_copyable_v<T>, "FByteArchive::ReadArray<T> requires a trivially copyable T");
            const int size = length * static_cast<int>(sizeof(T));
            CheckReadSize(size);
            std::vector<T> result(static_cast<size_t>(length));
            if (length > 0) std::memcpy(result.data(), _data.data() + Position, static_cast<size_t>(size));
            Position += size;
            return result;
        }

        template <typename T>
        void ReadArray(std::vector<T>& array)
        {
            if (array.empty()) return;
            const int size = static_cast<int>(array.size()) * static_cast<int>(sizeof(T));
            CheckReadSize(size);
            std::memcpy(array.data(), _data.data() + Position, static_cast<size_t>(size));
            Position += size;
        }

    private:
        std::vector<uint8_t> _data;
        std::string _name;
    };
}
