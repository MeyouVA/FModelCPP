// Ported from CUE4Parse/UE4/FMod/Utils/EventNodesResolver.cs
// Walks an event's node graph (timeline -> trigger boxes / parameter layouts / transitions / instruments)
// to collect the waveform samples it references.
//
// Difference from C#: the terminal value in C# is a decoded Fmod5Sharp FmodSample. That decoder is out of
// scope (see FModSoundBank.h), so this port resolves to FWaveformRef -- the (SoundBankIndex, SubsoundIndex)
// pair a WaveformResourceNode points at -- with the bank's parsed SampleCount used as the bounds check that
// C# does against bank.Samples.Count. The graph traversal itself is a faithful port.
//
// The #if DEBUG LogMissingSamples / GetUnreferencedSamplesWithGuids helpers are omitted (no logging layer).
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../FModReader.h"
#include "../Nodes/EventNode.h"
#include "../Nodes/TimelineNode.h"
#include "../Nodes/ParameterLayoutNode.h"
#include "../Nodes/WaveformResourceNode.h"
#include "../Nodes/Instruments/BaseInstrumentNode.h"
#include "../Nodes/Instruments/MultiInstrumentNode.h"
#include "../Nodes/Instruments/ScattererInstrumentNode.h"
#include "../Nodes/Instruments/WaveformInstrumentNode.h"
#include "../Nodes/Transitions/BaseTransitionNode.h"
#include "../Nodes/Transitions/TransitionRegionNode.h"

namespace CUE4Parse::UE4::FMod::Utils
{
    struct FWaveformRef
    {
        int32_t SoundBankIndex = 0;
        int32_t SubsoundIndex = 0;

        bool operator==(const FWaveformRef& o) const
        {
            return SoundBankIndex == o.SoundBankIndex && SubsoundIndex == o.SubsoundIndex;
        }
    };

    class EventNodesResolver
    {
    public:
        static std::unordered_map<Objects::FModGuid, std::vector<FWaveformRef>>
        TryResolveAudioEvents(const FModReader& reader, bool& allWaveformsResolved)
        {
            std::unordered_map<Objects::FModGuid, std::vector<FWaveformRef>> result;
            int totalExpected = static_cast<int>(reader.WavEntries.size());

            for (const auto& kvp : reader.EventNodes)
                result[kvp.first] = ResolveEventNodesWithAudio(reader, *kvp.second);

            // Distinct count of resolved refs across all events.
            std::unordered_set<int64_t> uniqueResolved;
            for (const auto& kvp : result)
                for (const auto& ref : kvp.second)
                    uniqueResolved.insert(PackRef(ref));

            allWaveformsResolved = totalExpected == static_cast<int>(uniqueResolved.size());
            return result;
        }

    private:
        static int64_t PackRef(const FWaveformRef& r)
        {
            return (static_cast<int64_t>(static_cast<uint32_t>(r.SoundBankIndex)) << 32) |
                   static_cast<uint32_t>(r.SubsoundIndex);
        }

        static std::vector<FWaveformRef> ResolveEventNodesWithAudio(const FModReader& reader,
                                                                    const Nodes::EventNode& evNode)
        {
            std::unordered_set<int64_t> resultSet;
            std::vector<FWaveformRef> result;
            std::unordered_set<Objects::FModGuid> visited;
            std::vector<Objects::FModGuid> stack;

            auto pushTimeline = [&stack](const Nodes::TimelineNode& tml)
            {
                for (const auto& box : tml.TriggerBoxes) stack.push_back(box.Guid);
                for (const auto& box : tml.TimeLockedTriggerBoxes) stack.push_back(box.Guid);
                for (const auto& m : tml.TimelineNamedMarkers) stack.push_back(m.BaseGuid);
                for (const auto& m : tml.TimelineTempoMarkers) stack.push_back(m.BaseGuid);
            };

            if (auto it = reader.TimelineNodes.find(evNode.TimelineGuid); it != reader.TimelineNodes.end())
                pushTimeline(*it->second);
            else
                stack.push_back(evNode.TimelineGuid);

            for (const auto& paramGuid : evNode.ParameterLayouts)
            {
                stack.push_back(paramGuid);
                if (auto it = reader.ParameterLayoutNodes.find(paramGuid); it != reader.ParameterLayoutNodes.end())
                {
                    for (const auto& instGuid : it->second->Instruments)
                        stack.push_back(instGuid);
                    for (const auto& tb : it->second->TriggerBoxes)
                        stack.push_back(tb.InstrumentGuid);
                }
            }

            for (const auto& inst : evNode.EventTriggeredInstruments)
                stack.push_back(inst);

            // destinationLookup: TransitionRegionNodes grouped by DestinationGuid.
            std::unordered_multimap<Objects::FModGuid, const Nodes::Transitions::TransitionRegionNode*> destinationLookup;
            for (const auto& kv : reader.TransitionNodes)
                if (auto* region = dynamic_cast<const Nodes::Transitions::TransitionRegionNode*>(kv.second.get()))
                    destinationLookup.emplace(region->DestinationGuid, region);

            while (!stack.empty())
            {
                Objects::FModGuid guid = stack.back();
                stack.pop_back();
                if (!visited.insert(guid).second)
                    continue;

                if (auto it = reader.TimelineNodes.find(guid); it != reader.TimelineNodes.end())
                    pushTimeline(*it->second);

                auto range = destinationLookup.equal_range(guid);
                for (auto rit = range.first; rit != range.second; ++rit)
                {
                    const auto* transTimeline = rit->second;
                    if (transTimeline->TransitionBody != nullptr)
                    {
                        for (const auto& fade : transTimeline->TransitionBody->FadeOverrides)
                        {
                            stack.push_back(fade.CurveGuid);
                            stack.push_back(fade.ControllerGuid);
                        }
                        for (const auto& box : transTimeline->TransitionBody->TriggeredTriggerBoxes)
                            stack.push_back(box.Guid);
                        for (const auto& box : transTimeline->TransitionBody->TimeLockedTriggerBoxes)
                            stack.push_back(box.Guid);
                    }
                }

                if (auto it = reader.InstrumentNodes.find(guid); it != reader.InstrumentNodes.end())
                {
                    Nodes::Instruments::BaseInstrumentNode* baseInstrNode = it->second.get();
                    if (baseInstrNode->InstrumentBody != nullptr)
                        stack.push_back(baseInstrNode->InstrumentBody->TimelineGuid);

                    if (auto* multiInst = dynamic_cast<Nodes::Instruments::MultiInstrumentNode*>(baseInstrNode);
                        multiInst != nullptr && multiInst->PlaylistBody != nullptr)
                    {
                        for (const auto& plEntry : multiInst->PlaylistBody->Entries)
                            stack.push_back(plEntry.Guid);
                    }
                    else if (auto* scatterInst = dynamic_cast<Nodes::Instruments::ScattererInstrumentNode*>(baseInstrNode);
                             scatterInst != nullptr && scatterInst->PlaylistBody != nullptr)
                    {
                        for (const auto& plEntry : scatterInst->PlaylistBody->Entries)
                            stack.push_back(plEntry.Guid);
                    }

                    if (auto* wavInstr = dynamic_cast<Nodes::Instruments::WaveformInstrumentNode*>(baseInstrNode))
                    {
                        auto we = reader.WavEntries.find(wavInstr->WaveformResourceGuid);
                        if (we != reader.WavEntries.end() &&
                            !reader.SoundBankData.empty() &&
                            we->second->SoundBankIndex < static_cast<int>(reader.SoundBankData.size()) &&
                            we->second->SubsoundIndex < reader.SoundBankData[we->second->SoundBankIndex].SampleCount)
                        {
                            FWaveformRef ref{ we->second->SoundBankIndex, we->second->SubsoundIndex };
                            if (resultSet.insert(PackRef(ref)).second)
                                result.push_back(ref);
                        }
                    }
                }
            }

            return result;
        }
    };
}
