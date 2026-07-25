// Ported from CUE4Parse/UE4/FMod/FModReader.cs -- see FModReader.h for the porting notes.
#include "FModReader.h"

#include <string>

#include "Objects/FParentContext.h"

// Metadata readers
#include "Metadata/FBankInfo.h"
#include "Metadata/HashData.h"
#include "Metadata/SoundDataInfo.h"
#include "Metadata/SoundTable.h"
#include "Metadata/StringTable.h"

// Node readers
#include "Nodes/ControllerNode.h"
#include "Nodes/ControllerOwnerNode.h"
#include "Nodes/CurveNode.h"
#include "Nodes/EventNode.h"
#include "Nodes/MappingNode.h"
#include "Nodes/ModulatorNode.h"
#include "Nodes/ParameterLayoutNode.h"
#include "Nodes/ParameterNode.h"
#include "Nodes/PlaylistNode.h"
#include "Nodes/PropertyNode.h"
#include "Nodes/SnapshotNode.h"
#include "Nodes/SoundDataNode.h"
#include "Nodes/TimelineNode.h"
#include "Nodes/VCANode.h"
#include "Nodes/WaveformResourceNode.h"
#include "Nodes/Buses/BaseBusNode.h"
#include "Nodes/Buses/BusNode.h"
#include "Nodes/Buses/GroupBusNode.h"
#include "Nodes/Buses/InputBusNode.h"
#include "Nodes/Buses/MasterBusNode.h"
#include "Nodes/Buses/OutputPortNode.h"
#include "Nodes/Buses/ReturnBusNode.h"
#include "Nodes/Effects/BaseEffectNode.h"
#include "Nodes/Effects/BuiltInEffectNode.h"
#include "Nodes/Effects/EffectNode.h"
#include "Nodes/Effects/ParameterizedEffectNode.h"
#include "Nodes/Effects/PluginEffectNode.h"
#include "Nodes/Effects/SendEffectNode.h"
#include "Nodes/Effects/SideChainEffectNode.h"
#include "Nodes/Effects/SpectralSideChainEffectNode.h"
#include "Nodes/Instruments/BaseInstrumentNode.h"
#include "Nodes/Instruments/CommandInstrumentNode.h"
#include "Nodes/Instruments/EffectInstrumentNode.h"
#include "Nodes/Instruments/EventInstrumentNode.h"
#include "Nodes/Instruments/InstrumentNode.h"
#include "Nodes/Instruments/MultiInstrumentNode.h"
#include "Nodes/Instruments/ProgrammerInstrumentNode.h"
#include "Nodes/Instruments/ScattererInstrumentNode.h"
#include "Nodes/Instruments/SilenceInstrumentNode.h"
#include "Nodes/Instruments/WaveformInstrumentNode.h"
#include "Nodes/Transitions/BaseTransitionNode.h"
#include "Nodes/Transitions/TransitionRegionNode.h"
#include "Nodes/Transitions/TransitionTimelineNode.h"

namespace CUE4Parse::UE4::FMod
{
    using Enums::ERIFFID;
    using Objects::FModGuid;
    using Objects::FParentContext;
    namespace Buses = Nodes::Buses;
    namespace Effects = Nodes::Effects;
    namespace Instruments = Nodes::Instruments;
    namespace Transitions = Nodes::Transitions;

    // --- static state ---
    Metadata::FFormatInfo FModReader::FormatInfo{};
    std::unique_ptr<Metadata::SoundDataInfo> FModReader::SoundDataInfo;
    std::optional<std::vector<uint8_t>> FModReader::EncryptionKey;

    FModReader::FModReader(Readers::FArchive& Ar, std::string bankName,
                           std::optional<std::vector<uint8_t>> encryptionKey)
        : BankName(std::move(bankName))
    {
        if (encryptionKey.has_value())
            EncryptionKey = std::move(encryptionKey);
        ParseHeader(Ar);
        ParseNodes(Ar, Ar.Position, Ar.Length);
    }

    FModReader::~FModReader() = default;

    Objects::FModGuid FModReader::GetBankGuid() const
    {
        return BankInfo ? BankInfo->BaseGuid : Objects::FModGuid();
    }

    void FModReader::ParseHeader(Readers::FArchive& Ar)
    {
        if (Ar.Length < 12)
            throw std::runtime_error("File too small to be a valid RIFF header");

        std::vector<uint8_t> riffBytes = Ar.ReadBytes(4);
        std::string riff(riffBytes.begin(), riffBytes.end());
        if (riff != "RIFF")
            throw std::runtime_error("Not a valid RIFF file");

        uint32_t riffSize = Ar.Read<uint32_t>();
        std::vector<uint8_t> typeBytes = Ar.ReadBytes(4);
        std::string fileType(typeBytes.begin(), typeBytes.end());
        if (fileType != "FEV ")
            throw std::runtime_error("Not a valid FMOD bank");

        int64_t expectedSize = static_cast<int64_t>(riffSize) + 8;
        int64_t actualSize = Ar.Length;

        if (actualSize < expectedSize)
            throw std::runtime_error("Truncated file");
        // (C# warns when the file is larger than the RIFF size; no logging layer in the port.)
    }

    void FModReader::ParseNodes(Readers::FArchive& Ar, int64_t start, int64_t end)
    {
        Ar.Position = start;

        std::vector<FParentContext> parentStack;
        bool visitedSoundNode = false;
        int soundDataIndex = 0;

        while (Ar.Position + 8 <= end)
        {
            int64_t nodeStart = Ar.Position;
            int32_t rawNodeValue = Ar.Read<int32_t>();

            // Shift to correct position if the node starts with a null terminator (usually end of a list).
            if ((rawNodeValue & 0xFF) == 0x00)
            {
                nodeStart = Ar.Position - 3;
                Ar.Position -= 3;
                rawNodeValue = Ar.Read<int32_t>();
            }

            ERIFFID nodeId = static_cast<ERIFFID>(rawNodeValue);
            uint32_t nodeSize = Ar.Read<uint32_t>();
            int64_t nextNode = nodeStart + 8 + nodeSize;

            if (nodeSize == 0)
            {
                Ar.Position = nextNode;
                continue;
            }

            switch (nodeId)
            {
                case ERIFFID::CHUNKID_FORMATINFO:
                    FormatInfo = Metadata::FFormatInfo(Ar);
                    break;

                case ERIFFID::CHUNKID_BANKINFO:
                    BankInfo = std::make_unique<Metadata::FBankInfo>(Ar);
                    break;

                case ERIFFID::CHUNKID_STRINGDATA:
                    StringTable = std::make_unique<Metadata::StringTable>(Ar);
                    break;

                case ERIFFID::CHUNKID_SOUNDTABLE:
                    SoundTable = std::make_unique<Metadata::SoundTable>(Ar);
                    break;

                case ERIFFID::CHUNKID_HASHDATA:
                    HashData = Metadata::HashData(Ar).Hashes;
                    break;

                case ERIFFID::CHUNKID_PLATFORM_INFO:
                    PlatformInfo = FModGuid(Ar);
                    break;

                case ERIFFID::CHUNKID_LIST: // List of sub-chunks
                {
                    (void) Ar.Read<int32_t>(); // listNodeId; not needed (custom structure)
                    ParseNodes(Ar, Ar.Position, nextNode);
                    break;
                }

                case ERIFFID::CHUNKID_LISTCOUNT:
                    (void) Ar.Read<uint32_t>(); // listCount; not needed (custom structure)
                    break;

                case ERIFFID::CHUNKID_OUTPUTPORTBODY:
                case ERIFFID::CHUNKID_RETURNBUSBODY:
                case ERIFFID::CHUNKID_INPUTBUSBODY:
                case ERIFFID::CHUNKID_GROUPBUSBODY:
                case ERIFFID::CHUNKID_MASTERBUSBODY:
                case ERIFFID::CHUNKID_BUS:
                    ParseBusNodes(Ar, nodeId, parentStack);
                    break;

                case ERIFFID::CHUNKID_SPECTRALSIDECHAINEFFECT:
                case ERIFFID::CHUNKID_BUILTINEFFECTBODY:
                case ERIFFID::CHUNKID_SENDEFFECTBODY:
                case ERIFFID::CHUNKID_SIDECHAINEFFECT:
                case ERIFFID::CHUNKID_PARAMETERIZEDEFFECT:
                case ERIFFID::CHUNKID_EFFECTBODY:
                case ERIFFID::CHUNKID_PLUGINEFFECTBODY:
                    ParseEffectNodes(Ar, nodeId, parentStack);
                    break;

                case ERIFFID::CHUNKID_SCATTERERINSTRUMENTBODY:
                case ERIFFID::CHUNKID_MULTIINSTRUMENTBODY:
                case ERIFFID::CHUNKID_PLAYLIST:
                case ERIFFID::CHUNKID_PROGRAMMERINSTRUMENTBODY:
                case ERIFFID::CHUNKID_COMMANDINSTRUMENTBODY:
                case ERIFFID::CHUNKID_WAVEFORMINSTRUMENTBODY:
                case ERIFFID::CHUNKID_EVENTINSTRUMENTBODY:
                case ERIFFID::CHUNKID_SILENCEINSTRUMENTBODY:
                case ERIFFID::CHUNKID_INSTRUMENT:
                case ERIFFID::CHUNKID_EFFECTINSTRUMENTBODY:
                    ParseInstrumentNodes(Ar, nodeId, parentStack);
                    break;

                case ERIFFID::CHUNKID_TRANSITIONREGIONBODY:
                case ERIFFID::CHUNKID_TRANSITIONTIMELINE:
                    ParseTransitionNodes(Ar, nodeId, parentStack);
                    break;

                case ERIFFID::CHUNKID_PROPERTY:
                {
                    auto node = std::make_unique<Nodes::PropertyNode>(Ar);
                    FModGuid g = node->MappingGuid;
                    PropertyNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_EVENTBODY:
                {
                    auto node = std::make_unique<Nodes::EventNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    EventNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_MODULATOR:
                case ERIFFID::CHUNKID_MODULATORBODY:
                {
                    auto node = std::make_unique<Nodes::ModulatorNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    ModulatorNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_PARAMETERBODY:
                {
                    auto node = std::make_unique<Nodes::ParameterNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    ParameterNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_PARAMETERLAYOUTBODY:
                {
                    auto node = std::make_unique<Nodes::ParameterLayoutNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    ParameterLayoutNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_WAVEFORMRESOURCE:
                {
                    auto node = std::make_unique<Nodes::WaveformResourceNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    WavEntries[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_TIMELINEBODY:
                {
                    auto node = std::make_unique<Nodes::TimelineNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    TimelineNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_SNAPSHOTBODY:
                {
                    auto node = std::make_unique<Nodes::SnapshotNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    SnapshotNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_VCA:
                case ERIFFID::CHUNKID_VCABODY:
                {
                    auto node = std::make_unique<Nodes::VCANode>(Ar);
                    FModGuid g = node->BaseGuid;
                    VCANodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_CURVE:
                {
                    auto node = std::make_unique<Nodes::CurveNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    CurveNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_CONTROLLEROWNER:
                {
                    Nodes::ControllerOwnerNode node(Ar);
                    for (const auto& c : node.Controllers)
                        ControllerOwnerNodes.push_back(c);
                    break;
                }

                case ERIFFID::CHUNKID_CONTROLLER:
                {
                    auto node = std::make_unique<Nodes::ControllerNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    ControllerNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_MAPPING:
                {
                    auto node = std::make_unique<Nodes::MappingNode>(Ar);
                    FModGuid g = node->BaseGuid;
                    MappingNodes[g] = std::move(node);
                    break;
                }

                case ERIFFID::CHUNKID_SOUNDDATAHEADER:
                    SoundDataInfo = std::make_unique<Metadata::SoundDataInfo>(Ar);
                    break;

                case ERIFFID::CHUNKID_SOUNDDATA:
                {
                    Nodes::SoundDataNode node(Ar, nodeStart, nodeSize, soundDataIndex);
                    visitedSoundNode = true;
                    soundDataIndex++;
                    if (node.SoundBank.has_value())
                        SoundBankData.push_back(std::move(*node.SoundBank));
                    break;
                }

                default:
                    // C# warns about an unknown chunk here; no logging layer in the port. The corrective
                    // seek below (Position != nextNode) skips it.
                    break;
            }

            // Stop if we already visited a sound node and this node is NOT a sound node. Some soundbanks
            // write duplicated FSB data outside of an SND chunk (there may be multiple SND chunks).
            if (visitedSoundNode && nodeId != ERIFFID::CHUNKID_SOUNDDATA)
                break;

            if (Ar.Position != nextNode)
                Ar.Position = nextNode; // C# also warns here (unless it's a LIST chunk)
        }
    }

    void FModReader::ParseBusNodes(Readers::FArchive& Ar, ERIFFID nodeId,
                                   std::vector<FParentContext>& parentStack)
    {
        switch (nodeId)
        {
            case ERIFFID::CHUNKID_OUTPUTPORTBODY:
            {
                auto node = std::make_unique<Buses::OutputPortNode>(Ar);
                FModGuid g = node->BaseGuid;
                BusNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_RETURNBUSBODY:
            {
                auto node = std::make_unique<Buses::ReturnBusNode>(Ar);
                FModGuid g = node->BaseGuid;
                BusNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_INPUTBUSBODY:
            {
                auto node = std::make_unique<Buses::InputBusNode>(Ar);
                FModGuid g = node->BaseGuid;
                BusNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_GROUPBUSBODY:
            {
                auto node = std::make_unique<Buses::GroupBusNode>(Ar);
                FModGuid g = node->BaseGuid;
                BusNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_MASTERBUSBODY:
            {
                auto node = std::make_unique<Buses::MasterBusNode>(Ar);
                FModGuid g = node->BaseGuid;
                BusNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_BUS:
            {
                if (!parentStack.empty())
                {
                    const FParentContext& busParent = parentStack.back();
                    if (busParent.NodeId == ERIFFID::CHUNKID_INPUTBUSBODY ||
                        busParent.NodeId == ERIFFID::CHUNKID_GROUPBUSBODY ||
                        busParent.NodeId == ERIFFID::CHUNKID_MASTERBUSBODY ||
                        busParent.NodeId == ERIFFID::CHUNKID_RETURNBUSBODY ||
                        busParent.NodeId == ERIFFID::CHUNKID_OUTPUTPORTBODY)
                    {
                        auto node = std::make_unique<Buses::BusNode>(Ar);
                        auto it = BusNodes.find(busParent.Guid);
                        if (it != BusNodes.end())
                            it->second->BusBody = std::move(node);
                        parentStack.pop_back();
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    void FModReader::ParseEffectNodes(Readers::FArchive& Ar, ERIFFID nodeId,
                                      std::vector<FParentContext>& parentStack)
    {
        switch (nodeId)
        {
            case ERIFFID::CHUNKID_BUILTINEFFECTBODY:
            {
                auto node = std::make_unique<Effects::BuiltInEffectNode>(Ar);
                FModGuid g = node->BaseGuid;
                EffectNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_PLUGINEFFECTBODY:
            {
                auto node = std::make_unique<Effects::PluginEffectNode>(Ar);
                FModGuid g = node->BaseGuid;
                EffectNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_PARAMETERIZEDEFFECT:
            {
                if (!parentStack.empty())
                {
                    const FParentContext paramEffectParent = parentStack.back();
                    if (paramEffectParent.NodeId == ERIFFID::CHUNKID_BUILTINEFFECTBODY ||
                        paramEffectParent.NodeId == ERIFFID::CHUNKID_PLUGINEFFECTBODY)
                    {
                        auto node = std::make_unique<Effects::ParameterizedEffectNode>(Ar);
                        auto it = EffectNodes.find(paramEffectParent.Guid);
                        if (it != EffectNodes.end())
                        {
                            if (auto* builtIn = dynamic_cast<Effects::BuiltInEffectNode*>(it->second.get()))
                                builtIn->ParamEffectBody = std::move(node);
                            else if (auto* plugin = dynamic_cast<Effects::PluginEffectNode*>(it->second.get()))
                                plugin->ParamEffectBody = std::move(node);
                        }
                        parentStack.pop_back();
                        parentStack.emplace_back(nodeId, paramEffectParent.Guid);
                    }
                }
                break;
            }
            case ERIFFID::CHUNKID_SPECTRALSIDECHAINEFFECT:
            {
                auto node = std::make_unique<Effects::SpectralSideChainEffectNode>(Ar);
                FModGuid g = node->BaseGuid;
                EffectNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_SENDEFFECTBODY:
            {
                auto node = std::make_unique<Effects::SendEffectNode>(Ar);
                FModGuid g = node->BaseGuid;
                EffectNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_SIDECHAINEFFECT:
            {
                auto node = std::make_unique<Effects::SideChainEffectNode>(Ar);
                FModGuid g = node->BaseGuid;
                EffectNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_EFFECTBODY:
            {
                if (!parentStack.empty())
                {
                    const FParentContext& effectParent = parentStack.back();
                    if (effectParent.NodeId == ERIFFID::CHUNKID_SENDEFFECTBODY ||
                        effectParent.NodeId == ERIFFID::CHUNKID_SIDECHAINEFFECT ||
                        effectParent.NodeId == ERIFFID::CHUNKID_SPECTRALSIDECHAINEFFECT ||
                        effectParent.NodeId == ERIFFID::CHUNKID_PARAMETERIZEDEFFECT)
                    {
                        auto node = std::make_unique<Effects::EffectNode>(Ar);
                        auto it = EffectNodes.find(effectParent.Guid);
                        if (it != EffectNodes.end())
                            it->second->EffectBody = std::move(node);
                        parentStack.pop_back();
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    void FModReader::ParseInstrumentNodes(Readers::FArchive& Ar, ERIFFID nodeId,
                                          std::vector<FParentContext>& parentStack)
    {
        switch (nodeId)
        {
            case ERIFFID::CHUNKID_SCATTERERINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::ScattererInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_MULTIINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::MultiInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_PLAYLIST:
            {
                if (!parentStack.empty())
                {
                    const FParentContext parentPlst = parentStack.back();
                    if (parentPlst.NodeId == ERIFFID::CHUNKID_SCATTERERINSTRUMENTBODY ||
                        parentPlst.NodeId == ERIFFID::CHUNKID_MULTIINSTRUMENTBODY)
                    {
                        auto node = std::make_unique<Nodes::PlaylistNode>(Ar);
                        auto it = InstrumentNodes.find(parentPlst.Guid);
                        if (it != InstrumentNodes.end())
                        {
                            if (auto* scatterer = dynamic_cast<Instruments::ScattererInstrumentNode*>(it->second.get()))
                                scatterer->PlaylistBody = std::move(node);
                            else if (auto* multi = dynamic_cast<Instruments::MultiInstrumentNode*>(it->second.get()))
                                multi->PlaylistBody = std::move(node);
                        }
                        parentStack.pop_back();
                        parentStack.emplace_back(nodeId, parentPlst.Guid);
                    }
                }
                break;
            }
            case ERIFFID::CHUNKID_PROGRAMMERINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::ProgrammerInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_COMMANDINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::CommandInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_WAVEFORMINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::WaveformInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_EVENTINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::EventInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_SILENCEINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::SilenceInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_EFFECTINSTRUMENTBODY:
            {
                auto node = std::make_unique<Instruments::EffectInstrumentNode>(Ar);
                FModGuid g = node->BaseGuid;
                InstrumentNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_INSTRUMENT:
            {
                if (!parentStack.empty())
                {
                    const FParentContext& parentInst = parentStack.back();
                    if (parentInst.NodeId == ERIFFID::CHUNKID_PROGRAMMERINSTRUMENTBODY ||
                        parentInst.NodeId == ERIFFID::CHUNKID_COMMANDINSTRUMENTBODY ||
                        parentInst.NodeId == ERIFFID::CHUNKID_WAVEFORMINSTRUMENTBODY ||
                        parentInst.NodeId == ERIFFID::CHUNKID_EVENTINSTRUMENTBODY ||
                        parentInst.NodeId == ERIFFID::CHUNKID_SILENCEINSTRUMENTBODY ||
                        parentInst.NodeId == ERIFFID::CHUNKID_PLAYLIST ||
                        parentInst.NodeId == ERIFFID::CHUNKID_EFFECTINSTRUMENTBODY)
                    {
                        auto node = std::make_unique<Instruments::InstrumentNode>(Ar);
                        auto it = InstrumentNodes.find(parentInst.Guid);
                        if (it != InstrumentNodes.end())
                            it->second->InstrumentBody = std::move(node);
                        parentStack.pop_back();
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    void FModReader::ParseTransitionNodes(Readers::FArchive& Ar, ERIFFID nodeId,
                                          std::vector<FParentContext>& parentStack)
    {
        switch (nodeId)
        {
            case ERIFFID::CHUNKID_TRANSITIONREGIONBODY:
            {
                auto node = std::make_unique<Transitions::TransitionRegionNode>(Ar);
                FModGuid g = node->BaseGuid;
                TransitionNodes[g] = std::move(node);
                parentStack.emplace_back(nodeId, g);
                break;
            }
            case ERIFFID::CHUNKID_TRANSITIONTIMELINE:
            {
                if (!parentStack.empty())
                {
                    const FParentContext& transParent = parentStack.back();
                    if (transParent.NodeId == ERIFFID::CHUNKID_TRANSITIONREGIONBODY)
                    {
                        auto node = std::make_unique<Transitions::TransitionTimelineNode>(Ar);
                        auto it = TransitionNodes.find(transParent.Guid);
                        if (it != TransitionNodes.end())
                            it->second->TransitionBody = std::move(node);
                        parentStack.pop_back();
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    void FModReader::Merge(FModReader& src)
    {
        auto mergeMap = [](auto& dst, auto& source)
        {
            for (auto& kv : source)
                dst[kv.first] = std::move(kv.second);
        };

        mergeMap(EventNodes, src.EventNodes);
        mergeMap(BusNodes, src.BusNodes);
        mergeMap(EffectNodes, src.EffectNodes);
        mergeMap(TimelineNodes, src.TimelineNodes);
        mergeMap(TransitionNodes, src.TransitionNodes);
        mergeMap(InstrumentNodes, src.InstrumentNodes);
        mergeMap(WavEntries, src.WavEntries);
        mergeMap(ParameterNodes, src.ParameterNodes);
        mergeMap(ModulatorNodes, src.ModulatorNodes);
        mergeMap(CurveNodes, src.CurveNodes);
        mergeMap(PropertyNodes, src.PropertyNodes);
        mergeMap(MappingNodes, src.MappingNodes);
        mergeMap(ParameterLayoutNodes, src.ParameterLayoutNodes);
        mergeMap(ControllerNodes, src.ControllerNodes);
        mergeMap(SnapshotNodes, src.SnapshotNodes);
        mergeMap(VCANodes, src.VCANodes);

        for (auto& bank : src.SoundBankData)
            SoundBankData.push_back(std::move(bank));
        for (const auto& c : src.ControllerOwnerNodes)
            ControllerOwnerNodes.push_back(c);
        if (!src.HashData.empty())
            HashData.insert(HashData.end(), src.HashData.begin(), src.HashData.end());
    }
}
