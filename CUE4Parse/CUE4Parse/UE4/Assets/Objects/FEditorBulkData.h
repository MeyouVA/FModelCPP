// Ported from CUE4Parse/UE4/Assets/Objects/FEditorBulkData.cs
// The editor-side ("virtualized") bulk data record. Cooked game packages never carry one -- UTexture only
// reads it when FStripDataFlags says editor data survived -- but the record is what an uncooked .uasset
// stores in place of an FByteBulkData, so the reader has to be able to step over it correctly.
//
// Deliberate differences from C#:
//   * The StoredInPackageTrailer branch needs Package.Trailer, which this port does not read yet (see the
//     note atop Package.h). C# warns and yields an empty payload when the package has no trailer; that is
//     exactly the path taken here, so the behaviour matches every package whose trailer is absent and
//     differs only for one that has one. Marked TODO rather than silently seeking to a wrong offset.
//   * No logging layer, so C#'s two Log calls are comments.
#pragma once

#include <cstdint>

#include "../Readers/FAssetArchive.h"
#include "../../Objects/Core/Compression/FCompressedBuffer.h"
#include "../../Objects/Core/Misc/FGuid.h"
#include "../../Objects/Core/Misc/FSHAHash.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::Objects::Core::Compression::FCompressedBuffer;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::Core::Misc::FSHAHash;

    // [Flags]
    enum class EFlags : uint32_t
    {
        /** No flags are set */
        None                      = 0,
        /** Is the data actually virtualized or not? */
        IsVirtualized             = 1 << 0,
        /** Does the package have access to a .upayload file? */
        HasPayloadSidecarFile     = 1 << 1,
        /** The bulkdata object is currently referencing a payload saved under old bulkdata formats */
        ReferencesLegacyFile      = 1 << 2,
        /** The legacy file being referenced is stored with Zlib compression format */
        LegacyFileIsCompressed    = 1 << 3,
        /** The payload should not have compression applied to it. It is assumed that the payload is already
            in some sort of compressed format, see the compression documentation above for more details. */
        DisablePayloadCompression = 1 << 4,
        /** The legacy file being referenced derived its key from guid and it should be replaced with a key-from-hash when saved */
        LegacyKeyWasGuidDerived   = 1 << 5,
        /** (Transient) The Guid has been registered with the BulkDataRegistry */
        HasRegistered             = 1 << 6,
        /** (Transient) The BulkData object is a copy used only to represent the id and payload; it does not communicate with the BulkDataRegistry, and will point DDC jobs toward the original BulkData */
        IsTornOff                 = 1 << 7,
        /** The bulkdata object references a payload stored in a WorkspaceDomain file  */
        ReferencesWorkspaceDomain = 1 << 8,
        /** The payload is stored in a package trailer, so the bulkdata object will have to poll the trailer to find the payload offset */
        StoredInPackageTrailer    = 1 << 9,
        /** The bulkdata object was cooked. */
        IsCooked                  = 1 << 10,
        /** (Transient) The package owning the bulkdata has been detached from disk and we can no longer load from it */
        WasDetached               = 1 << 11
    };

    inline bool HasFlag(EFlags value, EFlags flag)
    {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag);
    }

    class FEditorBulkData
    {
    public:
        EFlags Flags = EFlags::None;
        FGuid BulkDataId;
        FSHAHash PayloadContentId;
        int64_t PayloadSize = 0;
        int64_t OffsetInFile = 0;
        FCompressedBuffer Payload;

        explicit FEditorBulkData(Readers::FAssetArchive& Ar)
        {
            Flags = Ar.Read<EFlags>();
            BulkDataId = Ar.Read<FGuid>();
            PayloadContentId = FSHAHash(Ar);
            PayloadSize = Ar.Read<int64_t>();
            if (HasFlag(Flags, EFlags::StoredInPackageTrailer))
            {
                // TODO: package trailers are not parsed yet, so the offset cannot be looked up. C# takes this
                // same branch ("BulkData marked as stored in package trailer, but package has no trailer")
                // whenever Ar.Owner is not a Package or its Trailer is null, and yields an empty payload.
                return;
            }

            OffsetInFile = Ar.Read<int64_t>();

            if (OffsetInFile == -1) return;

            const int64_t savedPos = Ar.Position;
            try
            {
                Ar.Position = OffsetInFile;
                Payload = FCompressedBuffer(Ar);
            }
            catch (...)
            {
                // C# logs "Failed to read to EditorBulkData payload at offset {OffsetInFile}" and gives up.
                Payload = FCompressedBuffer();
            }
            Ar.Position = savedPos; // C#'s finally
        }
    };
}
