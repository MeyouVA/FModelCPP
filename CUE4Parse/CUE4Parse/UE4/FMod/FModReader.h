// Ported from CUE4Parse/UE4/FMod/FModReader.cs
// The RIFF chunk walker for an FMOD .bank ("FEV ") soundbank. Reads a flat tree of (4-byte tag,
// 4-byte size, payload) chunks, dispatching each to the matching node/metadata reader, and maintains a
// parent stack so a "body" chunk followed by its detail chunk (bus/effect/instrument/transition) are
// stitched together.
//
// Deliberate differences from the C# source, all forced by C++ / by the missing external decoder:
//   * C#'s System.IO.BinaryReader (little-endian) becomes Readers::FArchive. IMPORTANT: C#'s
//     BinaryReader.ReadBoolean/ReadByte read exactly ONE byte, whereas FArchive::ReadBoolean reads a
//     4-byte int. The port therefore never calls FArchive::ReadBoolean here -- it uses
//     `Ar.Read<uint8_t>() != 0` for a bool and `Ar.Read<uint8_t>()` for a byte. Ar.BaseStream.Position /
//     .Length map to Ar.Position / Ar.Length.
//   * The static state (Version, FormatInfo, SoundDataInfo, EncryptionKey) is kept as static members,
//     matching C#'s static fields -- these are global-per-parse and every node reader consults Version().
//   * The node dictionaries hold unique_ptr with FORWARD-DECLARED node types, so this header (which every
//     node includes for Version()/the read helpers) does not itself include the node headers -- that is
//     what breaks the C# circular reference cleanly. FModReader.cpp includes them and defines the dtor.
//   * SoundBankData holds the raw-bytes FModSoundBank stub (see FModSoundBank.h); the Fmod5Sharp sample
//     decode and the JSON converter are dropped.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Readers/FArchive.h"
#include "FModSoundBank.h"
#include "Objects/FUInt24.h"
#include "Objects/FModGuid.h"
#include "Metadata/FFormatInfo.h"
#include "Metadata/FHashInfo.h"
#include "Enums/ERIFFID.h"

namespace CUE4Parse::UE4::FMod
{
    // Forward declarations: this header must not pull in the node/metadata readers (they include it).
    namespace Metadata { class SoundDataInfo; class StringTable; class SoundTable; struct FBankInfo; }
    namespace Nodes
    {
        class EventNode; class TimelineNode; class ParameterNode; class ModulatorNode;
        class ParameterLayoutNode; class WaveformResourceNode; class SnapshotNode; class VCANode;
        class CurveNode; class PropertyNode; class MappingNode; class ControllerNode;
        namespace Buses { class BaseBusNode; }
        namespace Effects { class BaseEffectNode; }
        namespace Instruments { class BaseInstrumentNode; }
        namespace Transitions { class BaseTransitionNode; }
    }
    namespace Objects { struct FParentContext; }

    class FModReader
    {
    public:
        std::string BankName;

        // --- static per-parse state (C# static fields) ---
        static Metadata::FFormatInfo FormatInfo;
        static std::unique_ptr<Metadata::SoundDataInfo> SoundDataInfo;
        static std::optional<std::vector<uint8_t>> EncryptionKey;

        static int Version() { return FormatInfo.FileVersion; }

        // --- per-instance parsed data ---
        std::unique_ptr<Metadata::StringTable> StringTable;
        std::unique_ptr<Metadata::SoundTable> SoundTable;
        std::unique_ptr<Metadata::FBankInfo> BankInfo;
        std::optional<Objects::FModGuid> PlatformInfo;
        std::vector<Metadata::FHashInfo> HashData;
        std::vector<FModSoundBank> SoundBankData;

        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::EventNode>> EventNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::Buses::BaseBusNode>> BusNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::Effects::BaseEffectNode>> EffectNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::TimelineNode>> TimelineNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::Transitions::BaseTransitionNode>> TransitionNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::Instruments::BaseInstrumentNode>> InstrumentNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::WaveformResourceNode>> WavEntries;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::ParameterNode>> ParameterNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::ModulatorNode>> ModulatorNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::CurveNode>> CurveNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::PropertyNode>> PropertyNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::MappingNode>> MappingNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::ParameterLayoutNode>> ParameterLayoutNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::ControllerNode>> ControllerNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::SnapshotNode>> SnapshotNodes;
        std::unordered_map<Objects::FModGuid, std::unique_ptr<Nodes::VCANode>> VCANodes;
        std::vector<Objects::FModGuid> ControllerOwnerNodes;

        FModReader(Readers::FArchive& Ar, std::string bankName,
                   std::optional<std::vector<uint8_t>> encryptionKey = std::nullopt);
        ~FModReader(); // out-of-line: unique_ptr to forward-declared node types

        Objects::FModGuid GetBankGuid() const;
        void Merge(FModReader& src);

        // ---- Global Readers (C# #region Global Readers) --------------------------------------------

        // Reads a variable-length count/size: an int16, and if its high bit is set, an extra uint16 that
        // supplies the upper 15 bits. Returns the widened value.
        static uint32_t ReadX16(Readers::FArchive& Ar)
        {
            int16_t signedLow = Ar.Read<int16_t>();
            uint16_t low = static_cast<uint16_t>(signedLow);
            uint32_t value = low;

            if ((low & 0x8000) != 0)
            {
                uint16_t high = Ar.Read<uint16_t>();
                value &= 0x7FFFu;
                value |= (static_cast<uint32_t>(high) << 15);
            }

            return value;
        }

        static std::string ReadString(Readers::FArchive& Ar)
        {
            uint32_t length = ReadX16(Ar);
            if (length == 0)
                return std::string();

            std::vector<uint8_t> bytes = Ar.ReadBytes(static_cast<int>(length));
            return std::string(bytes.begin(), bytes.end());
        }

        // A length-prefixed list whose count is ReadX16 >> 1, with a per-element uint16 payload size.
        template <typename T>
        static std::vector<T> ReadVersionedElemListImp(Readers::FArchive& Ar)
        {
            uint32_t raw = ReadX16(Ar);
            int count = static_cast<int>(raw >> 1);
            if (count <= 0) return {};

            std::vector<T> result;
            result.reserve(static_cast<size_t>(count));
            for (int i = 0; i < count; i++)
            {
                (void) Ar.Read<uint16_t>(); // Payload size
                result.emplace_back(Ar);
            }
            return result;
        }

        template <typename Getter, typename T = std::invoke_result_t<Getter&, Readers::FArchive&>>
        static std::vector<T> ReadVersionedElemListImp(Readers::FArchive& Ar, Getter getter)
        {
            uint32_t raw = ReadX16(Ar);
            int count = static_cast<int>(raw >> 1);
            if (count <= 0) return {};

            std::vector<T> result;
            result.reserve(static_cast<size_t>(count));
            for (int i = 0; i < count; i++)
            {
                (void) Ar.Read<uint16_t>(); // Payload size
                result.push_back(getter(Ar));
            }
            return result;
        }

        // Same, but a single uint16 payload size precedes the whole list rather than each element.
        template <typename T>
        static std::vector<T> ReadElemListImp(Readers::FArchive& Ar)
        {
            uint32_t raw = ReadX16(Ar);
            int count = static_cast<int>(raw >> 1);
            if (count <= 0) return {};

            std::vector<T> result;
            result.reserve(static_cast<size_t>(count));
            (void) Ar.Read<uint16_t>(); // Payload size
            for (int i = 0; i < count; i++)
                result.emplace_back(Ar);
            return result;
        }

        template <typename Getter, typename T = std::invoke_result_t<Getter&, Readers::FArchive&>>
        static std::vector<T> ReadElemListImp(Readers::FArchive& Ar, Getter getter)
        {
            uint32_t raw = ReadX16(Ar);
            int count = static_cast<int>(raw >> 1);
            if (count <= 0) return {};

            std::vector<T> result;
            result.reserve(static_cast<size_t>(count));
            (void) Ar.Read<uint16_t>(); // Payload size
            for (int i = 0; i < count; i++)
                result.push_back(getter(Ar));
            return result;
        }

        static std::vector<Objects::FUInt24> ReadSimpleArray24(Readers::FArchive& Ar)
        {
            uint32_t count = ReadX16(Ar);
            if (count == 0) return {};

            int totalBytes = static_cast<int>(count) * 3; // 3 bytes per entry
            std::vector<uint8_t> raw = Ar.ReadBytes(totalBytes);

            std::vector<Objects::FUInt24> arr;
            arr.reserve(count);
            int o = 0;
            for (uint32_t i = 0; i < count; i++)
            {
                uint32_t v = static_cast<uint32_t>(raw[o]) |
                             (static_cast<uint32_t>(raw[o + 1]) << 8) |
                             (static_cast<uint32_t>(raw[o + 2]) << 16);
                o += 3;
                arr.emplace_back(v);
            }
            return arr;
        }

    private:
        void ParseHeader(Readers::FArchive& Ar);
        void ParseNodes(Readers::FArchive& Ar, int64_t start, int64_t end);
        void ParseBusNodes(Readers::FArchive& Ar, Enums::ERIFFID nodeId,
                           std::vector<Objects::FParentContext>& parentStack);
        void ParseEffectNodes(Readers::FArchive& Ar, Enums::ERIFFID nodeId,
                              std::vector<Objects::FParentContext>& parentStack);
        void ParseInstrumentNodes(Readers::FArchive& Ar, Enums::ERIFFID nodeId,
                                  std::vector<Objects::FParentContext>& parentStack);
        void ParseTransitionNodes(Readers::FArchive& Ar, Enums::ERIFFID nodeId,
                                  std::vector<Objects::FParentContext>& parentStack);
    };
}
