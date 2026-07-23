// Ported from CUE4Parse/UE4/IO/Objects/FIoStatus.cs (the exception subset the reader layer uses).
#pragma once

#include <string>

#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::IO::Objects
{
    enum class EIoErrorCode
    {
        Ok,
        Unknown,
        InvalidCode,
        Cancelled,
        FileOpenFailed,
        FileNotOpen,
        ReadError,
        WriteError,
        NotFound,
        CorruptToc,
        UnknownChunkID,
        InvalidParameter,
        SignatureError,
        InvalidEncryptionKey,
    };

    class FIoStatusException : public Exceptions::ParserException
    {
    public:
        EIoErrorCode ErrorCode;

        FIoStatusException(EIoErrorCode errorCode, const std::string& message)
            : ParserException(message), ErrorCode(errorCode) {}
    };
}
