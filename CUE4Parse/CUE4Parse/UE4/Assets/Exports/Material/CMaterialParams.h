// Ported from CUE4Parse/UE4/Assets/Exports/Material/CMaterialParams.cs
// The legacy (v1) material parameter bag: nine named texture slots plus the scalar/colour tweaks the old
// exporter needed. Populated by each UUnrealMaterial subclass's GetParams override; UTexture's is empty, so
// in this slice the type exists only so AppendReferencedTextures compiles and behaves as C# does.
//
// Deliberate difference from C#: the texture slots are non-owning pointers. C# holds GC references to
// materials the caller already owns (they are exports living in a package), so a raw pointer is the same
// lifetime story with the ownership made explicit -- the bag never outlives the package it points into.
#pragma once

#include <optional>
#include <vector>

#include "EMobileSpecularMask.h"
#include "ETextureChannel.h"
#include "../../../Objects/Core/Math/FLinearColor.h"

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    using CUE4Parse::UE4::Objects::Core::Math::FLinearColor;

    class UUnrealMaterial;

    class CMaterialParams
    {
    public:
        // textures
        UUnrealMaterial* Diffuse = nullptr;
        UUnrealMaterial* Normal = nullptr;
        UUnrealMaterial* Specular = nullptr;
        UUnrealMaterial* SpecPower = nullptr;
        UUnrealMaterial* Opacity = nullptr;
        UUnrealMaterial* Emissive = nullptr;
        UUnrealMaterial* Cube = nullptr;
        UUnrealMaterial* Mask = nullptr; // multiple mask textures baked into a single one
        UUnrealMaterial* Misc = nullptr; // M

        bool IsTransparent = false;
        bool HasTopDiffuseTexture = false;
        bool HasTopEmissiveTexture = false;

        float RoughnessValue = 1.0f;
        float MetallicValue = 0.0f;
        float SpecularValue = 0.0f;

        // channels (used with Mask texture)
        ETextureChannel EmissiveChannel = ETextureChannel::TC_NONE;
        ETextureChannel SpecularMaskChannel = ETextureChannel::TC_NONE;
        ETextureChannel SpecularPowerChannel = ETextureChannel::TC_NONE;
        ETextureChannel CubemapMaskChannel = ETextureChannel::TC_NONE;

        // colors
        std::optional<FLinearColor> DiffuseColor;
        std::optional<FLinearColor> EmissiveColor; // light-blue color

        // mobile
        bool UseMobileSpecular = false;
        float MobileSpecularPower = 0.0f;

        EMobileSpecularMask MobileSpecularMask = EMobileSpecularMask::MSM_Constant;

        // tweaks
        bool SpecularFromAlpha = false;
        bool OpacityFromAlpha = false;

        bool IsNull() const
        {
            return Diffuse == nullptr && Normal == nullptr && Specular == nullptr && SpecPower == nullptr &&
                   Opacity == nullptr && Emissive == nullptr && Cube == nullptr && Mask == nullptr && Misc == nullptr;
        }

        void AppendAllTextures(std::vector<UUnrealMaterial*>& outTextures) const
        {
            if (Diffuse != nullptr) outTextures.push_back(Diffuse);
            if (Normal != nullptr) outTextures.push_back(Normal);
            if (Specular != nullptr) outTextures.push_back(Specular);
            if (SpecPower != nullptr) outTextures.push_back(SpecPower);
            if (Opacity != nullptr) outTextures.push_back(Opacity);
            if (Emissive != nullptr) outTextures.push_back(Emissive);
            if (Cube != nullptr) outTextures.push_back(Cube);
            if (Mask != nullptr) outTextures.push_back(Mask);
            if (Misc != nullptr) outTextures.push_back(Misc);
        }
    };
}
