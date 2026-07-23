// Ported from CUE4Parse/UE4/Exceptions/InvalidAesKeyException.cs
// The innerException parameter is dropped: C++ exception chaining (std::nested_exception) is a different
// mechanism, and every throw site here only ever passes a message.
#pragma once

#include <string>

#include "ParserException.h"

namespace CUE4Parse::UE4::Exceptions
{
    class InvalidAesKeyException : public ParserException
    {
    public:
        explicit InvalidAesKeyException(const std::string& message = "") : ParserException(message) {}
        InvalidAesKeyException(const Readers::FArchive& reader, const std::string& message = "")
            : ParserException(reader, message) {}
    };
}
