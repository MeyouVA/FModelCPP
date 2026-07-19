// Ported from CUE4Parse/UE4/Exceptions/ParserException.cs (reader-formatting constructor).
#include "ParserException.h"
#include "../Readers/FArchive.h"

#include <cmath>
#include <sstream>

namespace CUE4Parse::UE4::Exceptions
{
    namespace
    {
        std::string FormatWithReader(const Readers::FArchive& reader, const std::string& message)
        {
            const double pct = reader.Length != 0
                ? std::round(static_cast<double>(reader.Position) / static_cast<double>(reader.Length) * 1000.0) / 10.0
                : 0.0;
            std::ostringstream os;
            os << message << "\n"
               << "FArchive Info: " << reader.Name()
               << " | Pos:" << reader.Position << " Length:" << reader.Length
               << " (" << pct << "% done)";
            return os.str();
        }
    }

    ParserException::ParserException(const Readers::FArchive& reader, const std::string& message)
        : std::runtime_error(FormatWithReader(reader, message))
    {
    }
}
