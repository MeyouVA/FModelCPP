// Ported from CUE4Parse/UE4/Wwise/Objects/AkPositioningParams.cs
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkPathMode.h"
#include "../Enums/EPositioningType.h"
#include "../Enums/Flags/EBitsPositioningFlags.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkPathMode;
    using CUE4Parse::UE4::Wwise::Enums::EPositioningType;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EBitsPositioningFlags;

    // The single messiest version-branching object in the bank format: where the "is this 3D" answer comes
    // from moves three times (an explicit bool, then a bit of the positioning flags at two different
    // offsets), and the automation/dynamic decision moves with it. C#'s structure is kept one-for-one.
    class AkPositioningParams
    {
    public:
        struct AkPathVertex
        {
            float X = 0;
            float Y = 0;
            float Z = 0;
            int32_t Duration = 0;

            AkPathVertex() = default;

            explicit AkPathVertex(FWwiseArchive& Ar)
            {
                X = Ar.Read<float>();
                Y = Ar.Read<float>();
                Z = Ar.Read<float>();
                Duration = Ar.Read<int32_t>();
            }
        };

        struct AkPathListItemOffset
        {
            uint32_t VerticesOffset = 0;
            uint32_t NumVertices = 0;

            AkPathListItemOffset() = default;

            explicit AkPathListItemOffset(FWwiseArchive& Ar)
            {
                VerticesOffset = Ar.Read<uint32_t>();
                NumVertices = Ar.Read<uint32_t>();
            }
        };

        struct AkPathListItem
        {
            float XRange = 0;
            float YRange = 0;
            float ZRange = 0;

            AkPathListItem() = default;

            explicit AkPathListItem(FWwiseArchive& Ar)
            {
                XRange = Ar.Read<float>();
                YRange = Ar.Read<float>();
                ZRange = Ar.Version > 89 ? Ar.Read<float>() : 0;
            }
        };

        EBitsPositioningFlags BitsPositioning = static_cast<EBitsPositioningFlags>(0);
        EAkPathMode PathMode = static_cast<EAkPathMode>(0);
        bool IsLooping = false;
        int32_t TransitionTime = 0;
        std::vector<AkPathVertex> Vertices;
        std::vector<AkPathListItemOffset> PlaylistItems;
        std::vector<AkPathListItem> PlaylistRanges;

        AkPositioningParams() = default;

        explicit AkPositioningParams(FWwiseArchive& Ar)
        {
            BitsPositioning = Ar.Read<EBitsPositioningFlags>();

            bool has3dPositioning = false;
            const bool hasPositioning = HasFlag(BitsPositioning, EBitsPositioningFlags::PositioningInfoOverrideParent);
            if (hasPositioning)
            {
                if (Ar.Version <= 56)
                {
                    Ar.Read<uint32_t>();
                    Ar.Read<float>();
                    Ar.Read<float>();
                }

                if (Ar.Version <= 72)
                {
                    has3dPositioning = Ar.ReadBool(); // cbIs3DPositioningAvailable
                    if (!has3dPositioning)
                        Ar.Read<uint8_t>(); // bIsPannerEnabled
                }
                else if (Ar.Version <= 89)
                {
                    const bool has2dPositioning = Ar.ReadBool(); // cbIs2DPositioningAvailable
                    has3dPositioning = Ar.ReadBool();            // cbIs3DPositioningAvailable
                    if (has2dPositioning)
                        Ar.Read<uint8_t>(); // bPositioningEnablePanner
                }
                else if (Ar.Version <= 122)
                {
                    has3dPositioning = HasFlag(BitsPositioning, EBitsPositioningFlags::Is3DPositioningAvailable_122);
                }
                else if (Ar.Version <= 129)
                {
                    has3dPositioning = HasFlag(BitsPositioning, EBitsPositioningFlags::Is3DPositioningAvailable_129);
                }
                else
                {
                    has3dPositioning = HasFlag(BitsPositioning, EBitsPositioningFlags::HasListenerRelativeRouting);
                }
            }

            if (hasPositioning && has3dPositioning)
            {
                EPositioningType positioningType = EPositioningType::Undefined;
                uint8_t flags3d = 0;
                if (Ar.Version <= 89)
                    positioningType = Ar.Read<EPositioningType>();
                else
                    flags3d = Ar.Read<uint8_t>();

                if (Ar.Version <= 89)
                {
                    Ar.Read<uint32_t>(); // AttenuationId
                    Ar.Read<uint8_t>();  // IsSpatialized
                }
                else if (Ar.Version <= 129)
                {
                    Ar.Read<uint32_t>(); // AttenuationId
                }

                auto [hasAutomation, isDynamic] = GetAutomationAndDynamicFlags(Ar, positioningType, flags3d, BitsPositioning);

                if (isDynamic)
                {
                    Ar.Read<uint8_t>(); // IsDynamic
                }

                if (hasAutomation)
                {
                    if (Ar.Version <= 89)
                    {
                        PathMode = static_cast<EAkPathMode>(Ar.Read<uint32_t>());
                        IsLooping = Ar.ReadBool();
                        TransitionTime = Ar.Read<int32_t>();
                        if (Ar.Version > 36)
                            Ar.Read<uint8_t>(); // bFollowOrientation
                    }
                    else
                    {
                        PathMode = Ar.Read<EAkPathMode>();
                        TransitionTime = Ar.Read<int32_t>();
                    }

                    const int numVertices = static_cast<int>(Ar.Read<uint32_t>());
                    Vertices = Ar.ReadArrayWith(numVertices, [&Ar] { return AkPathVertex(Ar); });

                    const uint32_t numPlaylistItems = Ar.Read<uint32_t>();
                    PlaylistItems = Ar.ReadArrayWith(static_cast<int>(numPlaylistItems),
                                                     [&Ar] { return AkPathListItemOffset(Ar); });
                    // The ranges array shares the item count but is absent entirely before version 37.
                    if (Ar.Version > 36)
                    {
                        PlaylistRanges = Ar.ReadArrayWith(static_cast<int>(numPlaylistItems),
                                                          [&Ar] { return AkPathListItem(Ar); });
                    }
                }
            }
        }

    private:
        static std::pair<bool, bool> GetAutomationAndDynamicFlags(FWwiseArchive& Ar, EPositioningType positioningType,
                                                                 int flags3d, EBitsPositioningFlags bitsPositioning)
        {
            bool hasAutomation, isDynamic;
            if (Ar.Version <= 72)
            {
                hasAutomation = positioningType == EPositioningType::UserDefined3D;
                isDynamic = positioningType == EPositioningType::GameDefined3D;
            }
            else if (Ar.Version <= 89)
            {
                const int eType = static_cast<int>(positioningType) & 3;
                hasAutomation = eType != 1;
                isDynamic = !hasAutomation;
            }
            else if (Ar.Version <= 122)
            {
                const int e3DPositionType122 = flags3d & 3;
                hasAutomation = e3DPositionType122 != 1;
                isDynamic = false;
            }
            else if (Ar.Version <= 126)
            {
                const int e3DPositionType126 = (flags3d >> 4) & 1;
                hasAutomation = e3DPositionType126 != 1;
                isDynamic = false;
            }
            else if (Ar.Version <= 129)
            {
                const int e3DPositionType129 = (flags3d >> 6) & 1;
                hasAutomation = e3DPositionType129 != 1;
                isDynamic = false;
            }
            else
            {
                // From 130 on the type comes out of the positioning flags rather than the 3D byte, and the
                // test flips from "!= 1" to "!= 0".
                const int e3DPositionType130 = (static_cast<int>(bitsPositioning) >> 5) & 3;
                hasAutomation = e3DPositionType130 != 0;
                isDynamic = false;
            }

            return {hasAutomation, isDynamic};
        }
    };
}
