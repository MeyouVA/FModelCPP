// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/FDialogueContext.cs
// [StructFallback]. Who is speaking and to whom; compared by resolved full name so that two contexts from
// different packages still match.
#pragma once

#include <string>
#include <vector>

#include "../../PropertyUtil.h"
#include "../../../IPackage.h"
#include "../../../ResolvedObject.h"
#include "../../../../Objects/UObject/FName.h"
#include "../../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    struct FDialogueContext
    {
        FPackageIndex Speaker;
        std::vector<FPackageIndex> Targets;

        FDialogueContext() = default;

        explicit FDialogueContext(const FStructFallback& fallback)
        {
            Speaker = PropertyUtil::GetOrDefault<FPackageIndex>(fallback, "Speaker", FPackageIndex());
            Targets = PropertyUtil::GetArray<FPackageIndex>(fallback, "Targets");
        }

        // C#'s `index.ResolvedObject?.GetFullName()`: an unresolvable index has no name, and C#'s
        // `!left?.Equals(right) ?? true` makes any such comparison false -- including null == null.
        static bool TryFullName(const FPackageIndex& index, std::string& out)
        {
            if (index.Owner == nullptr) return false;
            ResolvedObject* resolved = index.Owner->ResolvePackageIndex(&index);
            if (resolved == nullptr) return false;
            out = resolved->GetFullName();
            return true;
        }

        // C#'s StringComparison.OrdinalIgnoreCase.
        static bool EqualsIgnoreCase(const std::string& a, const std::string& b)
        {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++)
            {
                const char ca = (a[i] >= 'a' && a[i] <= 'z') ? static_cast<char>(a[i] - ('a' - 'A')) : a[i];
                const char cb = (b[i] >= 'a' && b[i] <= 'z') ? static_cast<char>(b[i] - ('a' - 'A')) : b[i];
                if (ca != cb) return false;
            }
            return true;
        }

        bool operator==(const FDialogueContext& other) const
        {
            std::string leftName, rightName;
            if (!TryFullName(Speaker, leftName)) return false;
            if (!TryFullName(other.Speaker, rightName) ||
                !EqualsIgnoreCase(leftName, rightName)) return false;

            if (Targets.size() != other.Targets.size()) return false;

            for (size_t i = 0; i < Targets.size(); i++)
            {
                std::string leftTarget, rightTarget;
                if (!TryFullName(Targets[i], leftTarget)) return false;
                if (!TryFullName(other.Targets[i], rightTarget) ||
                    !EqualsIgnoreCase(leftTarget, rightTarget)) return false;
            }

            return true;
        }

        bool operator!=(const FDialogueContext& other) const { return !(*this == other); }
    };
}
