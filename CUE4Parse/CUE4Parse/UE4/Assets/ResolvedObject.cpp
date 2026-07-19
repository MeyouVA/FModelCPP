// Ported from the ResolvedObject path-name helpers in CUE4Parse/UE4/Assets/AbstractUePackage.cs.
#include "ResolvedObject.h"

#include "IPackage.h"

namespace CUE4Parse::UE4::Assets
{
    std::string ResolvedObject::GetFullName(bool includeOuterMostName, bool includeClassPackage) const
    {
        std::string result;
        GetFullName(includeOuterMostName, result, includeClassPackage);
        return result;
    }

    void ResolvedObject::GetFullName(bool includeOuterMostName, std::string& result, bool includeClassPackage) const
    {
        if (ResolvedObject* clazz = Class())
            result += includeClassPackage ? clazz->GetPathName() : clazz->Name().Text();
        result += '\'';
        GetPathName(includeOuterMostName, result);
        result += '\'';
    }

    std::string ResolvedObject::GetPathName(bool includeOuterMostName) const
    {
        std::string result;
        GetPathName(includeOuterMostName, result);
        return result;
    }

    void ResolvedObject::GetPathName(bool includeOuterMostName, std::string& result) const
    {
        if (ResolvedObject* objOuter = Outer())
        {
            ResolvedObject* objOuterOuter = objOuter->Outer();
            if (objOuterOuter != nullptr || includeOuterMostName)
            {
                objOuter->GetPathName(includeOuterMostName, result);
                // ':' when the outer's outer is itself the outermost (its Outer is null), else '.'.
                result += (objOuterOuter != nullptr && objOuterOuter->Outer() == nullptr) ? ':' : '.';
            }
        }
        result += Name().Text();
    }

    FName ResolvedPackageObject::Name() const
    {
        return FName(Package->GetName());
    }
}
