// Ported from CUE4Parse/UE4/Objects/UObject/UField.cs.
#include "UField.h"

#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using Base = CUE4Parse::UE4::Assets::Exports::UObject;

    void UField::Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos)
    {
        Base::Deserialize(Ar, validPos);
        // Modern assets (RemoveUField_Next present) do not serialize Next. TODO: FFrameworkObjectVersion gate.
    }
}
