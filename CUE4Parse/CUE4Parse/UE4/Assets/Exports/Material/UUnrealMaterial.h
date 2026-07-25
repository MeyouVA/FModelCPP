// Ported from CUE4Parse/UE4/Assets/Exports/Material/UUnrealMaterial.cs
// The abstract base shared by every material AND every texture: from the exporter's point of view a texture
// is just a material that resolves to itself. UTexture derives from this, which is why the type has to exist
// before the texture tree can be ported at all.
//
// Deliberate difference from C#: CMaterialParams2 is only forward-declared. The v2 parameter bag is 350
// lines of texture-name tables that belong with the material tree, and nothing in the texture tree touches
// its members -- UTexture's GetParams overrides are empty in the C# too ("Default empty method // ???").
// An incomplete type is enough for a reference parameter, so the signature stays exact and the file that
// fills it in arrives with the material slice. TODO: port CMaterialParams2.
#pragma once

#include <vector>

#include "CMaterialParams.h"
#include "EMaterialFormat.h"
#include "../UObject.h"

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    class CMaterialParams2;

    class UUnrealMaterial : public UObject
    {
    public:
        virtual bool IsTextureCube() const { return false; }

        virtual void GetParams(CMaterialParams& parameters) = 0;
        virtual void GetParams(CMaterialParams2& parameters, EMaterialFormat format) = 0;

        virtual void AppendReferencedTextures(std::vector<UUnrealMaterial*>& outTextures, bool onlyRendered)
        {
            (void) onlyRendered; // C# ignores it here too; only the overrides use it.
            CMaterialParams parameters;
            GetParams(parameters);
            parameters.AppendAllTextures(outTextures);
        }
    };
}
