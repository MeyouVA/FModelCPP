// Ported from CUE4Parse/UE4/Objects/Core/Misc/FEngineVersion.cs
#pragma once

#include <string>

#include "FEngineVersionBase.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Misc
{
    class FEngineVersion : public FEngineVersionBase
    {
    public:
        // Branch name. Stored with '/' encoded as '+'; Branch() decodes it back.
        std::string Branch() const { return Replace(_branch, '+', '/'); }

        explicit FEngineVersion(Readers::FArchive& Ar) : FEngineVersionBase(Ar)
        {
            _branch = Ar.ReadFString();
        }

        FEngineVersion(uint16_t major, uint16_t minor, uint16_t patch, uint32_t changelist, const std::string& branch)
            : FEngineVersionBase(major, minor, patch, changelist), _branch(Replace(branch, '/', '+')) {}

        void Set(uint16_t major, uint16_t minor, uint16_t patch, uint32_t changelist, const std::string& branch)
        {
            Major = major;
            Minor = minor;
            Patch = patch;
            _changelist = changelist;
            _branch = Replace(branch, '/', '+');
        }

        std::string ToString(EVersionComponent lastComponent) const
        {
            std::string result = std::to_string(Major);
            if (lastComponent >= EVersionComponent::Minor)
            {
                result += "." + std::to_string(Minor);
                if (lastComponent >= EVersionComponent::Patch)
                {
                    result += "." + std::to_string(Patch);
                    if (lastComponent >= EVersionComponent::Changelist)
                    {
                        result += "-" + std::to_string(Changelist());
                        if (lastComponent >= EVersionComponent::Branch && !_branch.empty())
                        {
                            result += "+" + _branch;
                        }
                    }
                }
            }
            return result;
        }

        std::string ToString() const { return ToString(EVersionComponent::Branch); }

    private:
        std::string _branch;

        static std::string Replace(const std::string& s, char from, char to)
        {
            std::string out = s;
            for (char& ch : out)
                if (ch == from) ch = to;
            return out;
        }
    };
}
