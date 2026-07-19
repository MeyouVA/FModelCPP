// Ported from CUE4Parse/UE4/Readers/FStreamArchive.cs
#include "FStreamArchive.h"

#include <stdexcept>

namespace CUE4Parse::UE4::Readers
{
    // ---- FStreamArchive ----

    FStreamArchive::FStreamArchive(std::string name, std::shared_ptr<std::istream> baseStream, VersionContainer versions)
        : FArchive(std::move(versions)), _baseStream(std::move(baseStream)), _name(std::move(name))
    {
        _baseStream->clear();
        _baseStream->seekg(0, std::ios::end);
        Length = static_cast<int64_t>(_baseStream->tellg());
        _baseStream->seekg(0, std::ios::beg);
    }

    int FStreamArchive::Read(uint8_t* buffer, int offset, int count)
    {
        _baseStream->clear();
        _baseStream->seekg(static_cast<std::streamoff>(Position), std::ios::beg);
        _baseStream->read(reinterpret_cast<char*>(buffer + offset), count);
        const int n = static_cast<int>(_baseStream->gcount());
        Position += n;
        return n;
    }

    int64_t FStreamArchive::Seek(int64_t offset, ESeekOrigin origin)
    {
        switch (origin)
        {
            case ESeekOrigin::Begin:   Position = offset; break;
            case ESeekOrigin::Current: Position = Position + offset; break;
            case ESeekOrigin::End:     Position = Length + offset; break;
            default: throw std::out_of_range("Invalid SeekOrigin");
        }
        return Position;
    }

    std::unique_ptr<FArchive> FStreamArchive::Clone() const
    {
        // Shares the underlying stream (matches the C# fallback case). Position is this archive's own state.
        auto clone = std::make_unique<FStreamArchive>(_name, _baseStream, Versions);
        clone->Position = Position;
        return clone;
    }

    // ---- FRandomAccessFileStreamArchive ----

    FRandomAccessFileStreamArchive::FRandomAccessFileStreamArchive(std::string filePath, VersionContainer versions)
        : FArchive(std::move(versions)), _name(std::move(filePath))
    {
        _file = std::make_shared<std::ifstream>(_name, std::ios::binary);
        if (!_file->is_open())
            throw std::runtime_error("Failed to open file: " + _name);
        _file->seekg(0, std::ios::end);
        Length = static_cast<int64_t>(_file->tellg());
        _file->seekg(0, std::ios::beg);
    }

    int FRandomAccessFileStreamArchive::Read(uint8_t* buffer, int offset, int count)
    {
        const int n = ReadAt(Position, buffer, offset, count);
        Position += n;
        return n;
    }

    int FRandomAccessFileStreamArchive::ReadAt(int64_t position, uint8_t* buffer, int offset, int count)
    {
        _file->clear();
        _file->seekg(static_cast<std::streamoff>(position), std::ios::beg);
        _file->read(reinterpret_cast<char*>(buffer + offset), count);
        return static_cast<int>(_file->gcount());
    }

    int64_t FRandomAccessFileStreamArchive::Seek(int64_t offset, ESeekOrigin origin)
    {
        switch (origin)
        {
            case ESeekOrigin::Begin:   Position = offset; break;
            case ESeekOrigin::Current: Position = Position + offset; break;
            case ESeekOrigin::End:     Position = Length + offset; break;
            default: throw std::out_of_range("Invalid SeekOrigin");
        }
        return Position;
    }

    std::unique_ptr<FArchive> FRandomAccessFileStreamArchive::Clone() const
    {
        auto clone = std::make_unique<FRandomAccessFileStreamArchive>(_name, Versions);
        clone->Position = Position;
        return clone;
    }
}
