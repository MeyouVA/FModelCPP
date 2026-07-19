// Ported from CUE4Parse/UE4/Readers/FStreamArchive.cs
// Archives backed by a std::istream / a file on disk.
//
// C# has three classes here: FStreamArchive (over Stream), FRandomAccessStreamArchive (over the
// GenericReader RandomAccessStream), and FRandomAccessFileStreamArchive (over a file handle).
// FRandomAccessStreamArchive is a thin wrapper around a package that doesn't exist in C++ and is
// redundant with the file archive, so it is intentionally omitted. TODO: revisit if a use appears.
//
// Model note: unlike C#, Position here is the archive's own authoritative field (as with the memory
// archives). Each read seeks the stream to Position first, so the stream's internal cursor need not
// be trusted between reads.
#pragma once

#include <cstdint>
#include <fstream>
#include <istream>
#include <memory>
#include <string>
#include <utility>

#include "FArchive.h"

namespace CUE4Parse::UE4::Readers
{
    class FStreamArchive : public FArchive
    {
    public:
        FStreamArchive(std::string name, std::shared_ptr<std::istream> baseStream, VersionContainer versions = VersionContainer());

        using FArchive::Read; // keep the inherited Read<T> template visible alongside the byte-source override

        int Read(uint8_t* buffer, int offset, int count) override;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        bool CanSeek() const override { return true; }
        const std::string& Name() const override { return _name; }
        std::unique_ptr<FArchive> Clone() const override;

    protected:
        std::shared_ptr<std::istream> _baseStream;
        std::string _name;
    };

    // Owns a file opened by path; positional reads via std::ifstream.
    class FRandomAccessFileStreamArchive : public FArchive
    {
    public:
        FRandomAccessFileStreamArchive(std::string filePath, VersionContainer versions = VersionContainer());

        bool IsOpen() const { return _file != nullptr && _file->is_open(); }

        using FArchive::Read; // keep the inherited Read<T> template visible alongside the byte-source override

        int Read(uint8_t* buffer, int offset, int count) override;
        int ReadAt(int64_t position, uint8_t* buffer, int offset, int count) override;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        bool CanSeek() const override { return true; }
        const std::string& Name() const override { return _name; }
        std::unique_ptr<FArchive> Clone() const override;

    private:
        std::shared_ptr<std::ifstream> _file;
        std::string _name;
    };
}
