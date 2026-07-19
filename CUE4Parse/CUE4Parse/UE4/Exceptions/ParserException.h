// Ported from CUE4Parse/UE4/Exceptions/ParserException.cs
#pragma once

#include <stdexcept>
#include <string>

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Exceptions
{
    class ParserException : public std::runtime_error
    {
    public:
        explicit ParserException(const std::string& message = "")
            : std::runtime_error(message) {}

        // Formats the reader's name/position/length into the message, like the C# overload.
        ParserException(const Readers::FArchive& reader, const std::string& message = "");
    };

    class MappingException : public ParserException
    {
    public:
        explicit MappingException(const std::string& message) : ParserException(message) {}
    };

    class VersionException : public ParserException
    {
    public:
        VersionException(const Readers::FArchive& reader, const std::string& message)
            : ParserException(reader, message) {}
    };
}
