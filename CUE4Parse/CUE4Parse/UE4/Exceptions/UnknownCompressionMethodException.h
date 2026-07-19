// Ported from CUE4Parse/UE4/Exceptions/UnknownCompressionMethodException.cs
#pragma once

#include <string>

#include "ParserException.h"

namespace CUE4Parse::UE4::Exceptions
{
    class UnknownCompressionMethodException : public ParserException
    {
    public:
        explicit UnknownCompressionMethodException(const std::string& message = "")
            : ParserException(message) {}

        UnknownCompressionMethodException(const Readers::FArchive& reader, const std::string& message = "")
            : ParserException(reader, message) {}
    };
}
