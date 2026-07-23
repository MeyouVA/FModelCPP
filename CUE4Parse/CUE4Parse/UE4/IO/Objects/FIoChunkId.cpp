#include "FIoChunkId.h"

#include "FPackageId.h"
#include "../IoStoreReader.h"
#include "../../Versions/EGame.h"
#include "../../VirtualFileSystem/IAesVfsReader.h"

namespace CUE4Parse::UE4::IO::Objects
{
    FPackageId FIoChunkId::AsPackageId() const
    {
        return FPackageId(ChunkId);
    }

    std::string FIoChunkId::GetExtension(const VirtualFileSystem::IAesVfsReader& reader) const
    {
        // C# round-trips the byte through the right enum's name and maps a few of them; the two enums are
        // folded into one switch per generation here.
        const bool ue5 = reader.Game() >= Versions::GAME_UE5_0;
        const bool isIoStore = dynamic_cast<const IoStoreReader*>(&reader) != nullptr;

        if (ue5)
        {
            switch (static_cast<EIoChunkType5>(ChunkType))
            {
                case EIoChunkType5::ExportBundleData: return isIoStore ? "uasset" : "uexp"; // umap
                case EIoChunkType5::BulkData: return "ubulk";
                case EIoChunkType5::OptionalBulkData: return "uptnl";
                case EIoChunkType5::MemoryMappedBulkData: return "m.ubulk";
                case EIoChunkType5::ShaderCodeLibrary: return "ushaderbytecode";
                case EIoChunkType5::ShaderCode: return "dxbc";
                case EIoChunkType5::Invalid: return "Invalid";
                case EIoChunkType5::ScriptObjects: return "ScriptObjects";
                case EIoChunkType5::ContainerHeader: return "ContainerHeader";
                case EIoChunkType5::ExternalFile: return "ExternalFile";
                case EIoChunkType5::PackageStoreEntry: return "PackageStoreEntry";
                case EIoChunkType5::DerivedData: return "DerivedData";
                case EIoChunkType5::EditorDerivedData: return "EditorDerivedData";
                case EIoChunkType5::PackageResource: return "PackageResource";
                default: return std::to_string(ChunkType); // C# falls back to the numeric value
            }
        }

        switch (static_cast<EIoChunkType>(ChunkType))
        {
            case EIoChunkType::ExportBundleData: return isIoStore ? "uasset" : "uexp"; // umap
            case EIoChunkType::BulkData: return "ubulk";
            case EIoChunkType::OptionalBulkData: return "uptnl";
            case EIoChunkType::MemoryMappedBulkData: return "m.ubulk";
            case EIoChunkType::Invalid: return "Invalid";
            case EIoChunkType::InstallManifest: return "InstallManifest";
            case EIoChunkType::LoaderGlobalMeta: return "LoaderGlobalMeta";
            case EIoChunkType::LoaderInitialLoadMeta: return "LoaderInitialLoadMeta";
            case EIoChunkType::LoaderGlobalNames: return "LoaderGlobalNames";
            case EIoChunkType::LoaderGlobalNameHashes: return "LoaderGlobalNameHashes";
            case EIoChunkType::ContainerHeader: return "ContainerHeader";
            default: return std::to_string(ChunkType);
        }
    }
}
