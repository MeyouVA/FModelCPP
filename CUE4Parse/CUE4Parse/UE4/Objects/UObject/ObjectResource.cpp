// Ported from CUE4Parse/UE4/Objects/UObject/ObjectResource.cs (FObjectExport/FObjectImport serialization).
#include "ObjectResource.h"

#include "../../Versions/ObjectVersion.h"
#include "../../Versions/EGame.h"
#include "../../Assets/IPackage.h"
#include "../../Assets/ResolvedObject.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using namespace CUE4Parse::UE4::Versions;

    std::string FPackageIndex::Name() const
    {
        if (Owner == nullptr) return "None";
        Assets::ResolvedObject* resolved = Owner->ResolvePackageIndex(this);
        return resolved != nullptr ? resolved->Name().Text() : "None";
    }

    std::string FPackageIndex::ToString() const
    {
        Assets::ResolvedObject* resolved = Owner != nullptr ? Owner->ResolvePackageIndex(this) : nullptr;
        return resolved != nullptr ? resolved->ToString() : std::to_string(Index);
    }

    FObjectExport::FObjectExport(FAssetArchive& Ar)
    {
        ClassIndex = FPackageIndex(Ar);
        SuperIndex = FPackageIndex(Ar);
        TemplateIndex = Ar.Ver() >= EUnrealEngineObjectUE4Version::TemplateIndex_IN_COOKED_EXPORTS ? FPackageIndex(Ar) : FPackageIndex();
        OuterIndex = FPackageIndex(Ar);
        ObjectName = Ar.ReadFName();
        ObjectFlags = Ar.Read<uint32_t>();

        if (Ar.Ver() < EUnrealEngineObjectUE4Version::e64BIT_EXPORTMAP_SERIALSIZES)
        {
            SerialSize = Ar.Read<int32_t>();
            SerialOffset = Ar.Read<int32_t>();
        }
        else
        {
            SerialSize = Ar.Read<int64_t>();
            SerialOffset = Ar.Read<int64_t>();
        }

        if (Ar.Game() >= GAME_UE4_0)
        {
            ForcedExport = Ar.ReadBoolean();
            NotForClient = Ar.ReadBoolean();
            NotForServer = Ar.ReadBoolean();
        }

        PackageGuid = Ar.Ver() < EUnrealEngineObjectUE5Version::REMOVE_OBJECT_EXPORT_PACKAGE_GUID ? Ar.Read<FGuid>() : FGuid();
        IsInheritedInstance = Ar.Ver() >= EUnrealEngineObjectUE5Version::TRACK_OBJECT_EXPORT_IS_INHERITED && Ar.ReadBoolean();
        PackageFlags = Ar.Read<uint32_t>();
        NotAlwaysLoadedForEditorGame = Ar.Ver() >= EUnrealEngineObjectUE4Version::LOAD_FOR_EDITOR_GAME && Ar.ReadBoolean();
        IsAsset = Ar.Ver() >= EUnrealEngineObjectUE4Version::COOKED_ASSETS_IN_EDITOR_SUPPORT && Ar.ReadBoolean();
        GeneratePublicHash = Ar.Ver() >= EUnrealEngineObjectUE5Version::OPTIONAL_RESOURCES && Ar.ReadBoolean();

        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::PRELOAD_DEPENDENCIES_IN_COOKED_EXPORTS)
        {
            FirstExportDependency = Ar.Read<int32_t>();
            SerializationBeforeSerializationDependencies = Ar.Read<int32_t>();
            CreateBeforeSerializationDependencies = Ar.Read<int32_t>();
            SerializationBeforeCreateDependencies = Ar.Read<int32_t>();
            CreateBeforeCreateDependencies = Ar.Read<int32_t>();
        }
        else
        {
            FirstExportDependency = -1;
            SerializationBeforeSerializationDependencies = 0;
            CreateBeforeSerializationDependencies = 0;
            SerializationBeforeCreateDependencies = 0;
            CreateBeforeCreateDependencies = 0;
        }

        if (!Ar.HasUnversionedProperties() && Ar.Ver() >= EUnrealEngineObjectUE5Version::SCRIPT_SERIALIZATION_OFFSET)
        {
            ScriptSerializationStartOffset = Ar.Read<int64_t>();
            ScriptSerializationEndOffset = Ar.Read<int64_t>();
        }
        else
        {
            ScriptSerializationStartOffset = 0;
            ScriptSerializationEndOffset = 0;
        }

        ClassName = ClassIndex.Name();
    }

    FObjectImport::FObjectImport(FAssetArchive& Ar)
    {
        ClassPackage = Ar.ReadFName();
        ClassName = Ar.ReadFName();
        OuterIndex = FPackageIndex(Ar);
        ObjectName = Ar.ReadFName();

        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::NON_OUTER_PACKAGE_IMPORT && !Ar.IsFilterEditorOnly())
        {
            PackageName = Ar.ReadFName();
        }

        if (Ar.Game() == GAME_RacingMaster) Ar.Position += 1;

        ImportOptional = Ar.Ver() >= EUnrealEngineObjectUE5Version::OPTIONAL_RESOURCES && Ar.ReadBoolean();
    }
}
