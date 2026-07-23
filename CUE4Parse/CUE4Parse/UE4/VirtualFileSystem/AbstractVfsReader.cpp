#include "AbstractVfsReader.h"

#include <algorithm>
#include <cstdlib>

#include "../Readers/FByteArchive.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::VirtualFileSystem
{
    namespace
    {
        bool EndsWithIgnoreCase(const std::string& s, const std::string& suffix)
        {
            if (s.size() < suffix.size()) return false;
            const size_t offset = s.size() - suffix.size();
            for (size_t i = 0; i < suffix.size(); ++i)
            {
                char a = s[offset + i];
                char b = suffix[i];
                if (a >= 'A' && a <= 'Z') a = static_cast<char>(a + 32);
                if (b >= 'A' && b <= 'Z') b = static_cast<char>(b + 32);
                if (a != b) return false;
            }
            return true;
        }
    }

    AbstractVfsReader::AbstractVfsReader(std::string path, Versions::VersionContainer versions)
        : _versions(std::move(versions)), _path(std::move(path))
    {
        std::string normalized = _path;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        _name = Utils::SubstringAfterLast(normalized, '/');
    }

    void AbstractVfsReader::ValidateMountPoint(std::string& mountPoint)
    {
        bool badMountPoint = mountPoint.rfind("../../..", 0) != 0;
        mountPoint = Utils::SubstringAfter(mountPoint, "../../..");
        if (mountPoint.empty() || mountPoint[0] != '/' || (mountPoint.size() > 1 && mountPoint[1] == '.'))
            badMountPoint = true;

        // C# logs a warning here; the port has no logging layer.
        if (badMountPoint) mountPoint = "/";

        mountPoint = mountPoint.substr(1);
        VerifyReadOrder();
    }

    void AbstractVfsReader::VerifyReadOrder()
    {
        _readOrder = GetPakOrderFromPakFilePath();
        if (!EndsWithIgnoreCase(_name, "_P.pak") && !EndsWithIgnoreCase(_name, "_P.utoc") &&
            !EndsWithIgnoreCase(_name, "_P.o.utoc"))
            return;

        uint32_t chunkVersionNumber = 1u;
        const size_t versionEndIndex = _name.rfind('_');
        if (versionEndIndex != std::string::npos && versionEndIndex > 0)
        {
            const size_t versionStartIndex = _name.rfind('_', versionEndIndex - 1);
            if (versionStartIndex != std::string::npos)
            {
                const std::string versionString = _name.substr(versionStartIndex + 1, versionEndIndex - (versionStartIndex + 1));
                // int.TryParse: the whole string must be an integer, so a partial parse does not count.
                try
                {
                    size_t consumed = 0;
                    const int chunkVersionSigned = std::stoi(versionString, &consumed);
                    if (consumed == versionString.size() && chunkVersionSigned >= 1)
                    {
                        // Increment by one so that the first patch file still gets more priority than the base pak file
                        chunkVersionNumber = static_cast<uint32_t>(chunkVersionSigned) + 1;
                    }
                }
                catch (const std::exception&) { /* not an integer — leave the default */ }
            }
        }
        _readOrder += 100 * static_cast<int64_t>(chunkVersionNumber);
    }

    int AbstractVfsReader::GetPakOrderFromPakFilePath()
    {
        // C# has the FPaths-based ladder commented out and returns a constant; kept as-is.
        return 3;
    }

    bool AbstractVfsReader::IsValidIndex(const std::vector<uint8_t>& testBytes)
    {
        Readers::FByteArchive reader(std::string(), testBytes);
        return IsValidIndex(reader);
    }

    bool AbstractVfsReader::IsValidIndex(Readers::FArchive& reader)
    {
        // Addition over C#: every read is range-checked first, and the whole probe is wrapped. This runs on
        // data decrypted with a key that is probably wrong, so the length it reads is arbitrary and the seek
        // routinely lands outside the buffer. C# gets a bounds exception there; the C++ memory archives
        // memcpy without checking, so an unchecked probe would read out of bounds instead. "Could not read"
        // is the same answer as "not a valid index", which is what the caller is asking.
        const auto canRead = [&reader](int64_t bytes) {
            return reader.Position >= 0 && reader.Position + bytes <= reader.Length;
        };

        try
        {
            if (!canRead(4)) return false;
            const int32_t mountPointLength = reader.Read<int32_t>();
            if (mountPointLength > MAX_MOUNTPOINT_TEST_LENGTH || mountPointLength < -MAX_MOUNTPOINT_TEST_LENGTH)
                return false;
            // Seek to where the null terminator of that string would be and check it really is 0.
            if (mountPointLength == 0) return canRead(1) && reader.Read<uint8_t>() == 0;
            if (mountPointLength < 0)
            {
                // UTF16. Note the C# arithmetic lands two code units *past* the terminator; kept verbatim,
                // because the buffer this probes (MountPointCheckBytes) is always long enough to absorb it
                // and changing it would change which keys are accepted.
                reader.Seek(-(static_cast<int64_t>(mountPointLength) - 1) * 2, Readers::ESeekOrigin::Current);
                return canRead(2) && reader.Read<int16_t>() == 0;
            }

            // UTF8
            reader.Seek(mountPointLength - 1, Readers::ESeekOrigin::Current);
            return canRead(1) && reader.Read<uint8_t>() == 0;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
}
