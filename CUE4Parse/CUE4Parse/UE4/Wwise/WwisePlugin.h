// Ported from CUE4Parse/UE4/Wwise/WwisePlugin.cs
// The plugin-id dispatch table: given the raw id read from a bank source, construct the matching
// IAkPluginParam and leave the archive exactly at the end of the declared payload.
//
// C# logs through Serilog on a parse failure and (in DEBUG) whenever a plugin has no handler or reads
// the wrong number of bytes. This port has no logging layer, so those diagnostics are dropped; the
// behaviour that matters -- swallow the exception, always seek to endPosition -- is kept.
#pragma once

#include <cstdint>
#include <memory>

#include "WwiseArchive.h"
#include "Enums/EAkCompanyID.h"
#include "Enums/EAkPluginId.h"
#include "Enums/EAkPluginType.h"

#include "Plugins/CAk3DAudioBedMixerFXParams.h"
#include "Plugins/CAkAsioParams.h"
#include "Plugins/CAkChannelRouterFXParams.h"
#include "Plugins/CAkCompressorFXParams.h"
#include "Plugins/CAkConvolutionReverbFXParams.h"
#include "Plugins/CAkDefaultSinkParams.h"
#include "Plugins/CAkDelayFXParams.h"
#include "Plugins/CAkExpanderFXParams.h"
#include "Plugins/CAkFDNReverbFXParams.h"
#include "Plugins/CAkFlangerFXParams.h"
#include "Plugins/CAkFxSrcAudioInputParams.h"
#include "Plugins/CAkFxSrcSilenceParams.h"
#include "Plugins/CAkFxSrcSineParams.h"
#include "Plugins/CAkGainFXParams.h"
#include "Plugins/CAkGranularSynthParams.h"
#include "Plugins/CAkGuitarDistortionFXParams.h"
#include "Plugins/CAkHarmonizerFXParams.h"
#include "Plugins/CAkImpacterParams.h"
#include "Plugins/CAkMeterFXParams.h"
#include "Plugins/CAkModalSynthParams.h"
#include "Plugins/CAkMotionGeneratorParams.h"
#include "Plugins/CAkMotionSourceParams.h"
#include "Plugins/CAkParameterEQFXParams.h"
#include "Plugins/CAkPeakLimiterFXParams.h"
#include "Plugins/CAkPitchShifterFXParams.h"
#include "Plugins/CAkRecorderADMFXParams.h"
#include "Plugins/CAkRecorderFXParams.h"
#include "Plugins/CAkReflectFXParams.h"
#include "Plugins/CAkRoomVerbFXParams.h"
#include "Plugins/CAkSidechainFXParams.h"
#include "Plugins/CAkSoundSeedWindParams.h"
#include "Plugins/CAkSoundSeedWooshParams.h"
#include "Plugins/CAkStereoDelayFXParams.h"
#include "Plugins/CAkSynthOneParams.h"
#include "Plugins/CAkSystemOutputParams.h"
#include "Plugins/CAkTimeStretchFXParams.h"
#include "Plugins/CAkToneGenParams.h"
#include "Plugins/CAkTremoloFXParams.h"
#include "Plugins/CMicrosoftHRTFSinkParams.h"
#include "Plugins/IAkPluginParam.h"
#include "Plugins/Auro/CAuroHPFXParams.h"
#include "Plugins/Auro/CAuroPannerParams.h"
#include "Plugins/Bitcrush/CBitcrushFXParams.h"
#include "Plugins/CrankcaseAudioREVModelPlayer/CREVSourceModelPlayerParams.h"
#include "Plugins/MasteringSuite/CMasteringSuiteFXParams.h"
#include "Plugins/McDSP/CMcDSPFutzBoxFXParams.h"
#include "Plugins/McDSP/CMcDSPLimiterFXParams.h"
#include "Plugins/MetaXRAudio/OculusEndpointParams.h"
#include "Plugins/Mindseye/MindseyePluginParams.h"
#include "Plugins/OculusSpatializer/COculusSpatializerFXParams.h"
#include "Plugins/PolyspectralMBC/CMBCRuntimeParams.h"
#include "Plugins/ResonanceAudio/ResonanceAudioParams.h"
#include "Plugins/atmoky/CatmokyEarsFXParams.h"
#include "Plugins/iZotope/CiZHybridReverbFXParams.h"
#include "Plugins/iZotope/CiZTrashBoxModelerFXParams.h"
#include "Plugins/iZotope/CiZTrashDelayFXParams.h"
#include "Plugins/iZotope/CiZTrashDistortionFXParams.h"
#include "Plugins/iZotope/CiZTrashDynamicsFXParams.h"
#include "Plugins/iZotope/CiZTrashFiltersFXParams.h"
#include "Plugins/iZotope/CiZTrashMultibandDistortionFXParams.h"

namespace CUE4Parse::UE4::Wwise
{
    using CUE4Parse::UE4::Wwise::Enums::AkCompanyID;
    using CUE4Parse::UE4::Wwise::Enums::EAkPluginId;
    using CUE4Parse::UE4::Wwise::Enums::EAkPluginType;
    using CUE4Parse::UE4::Wwise::Plugins::IAkPluginParam;

    // A raw plugin id, decomposed on demand: the low nibble is the plugin type, the next byte the
    // company, and the whole word identifies the plugin.
    struct AkPlugin
    {
        uint32_t _raw = 0;

        AkPlugin() = default;
        explicit AkPlugin(uint32_t rawId) : _raw(rawId) {}

        static AkPlugin None() { return AkPlugin(UINT32_MAX); }

        bool IsValid() const { return _raw != UINT32_MAX && _raw != 0; }

        EAkPluginId PluginId() const { return static_cast<EAkPluginId>(_raw); }
        AkCompanyID CompanyId() const
        {
            return IsValid() ? static_cast<AkCompanyID>((_raw >> 4) & 0xFF) : AkCompanyID::Audiokinetic;
        }
        EAkPluginType Type() const
        {
            return IsValid() ? static_cast<EAkPluginType>(_raw & 0xF) : EAkPluginType::None;
        }
    };

    class WwisePlugin
    {
    public:
        static std::unique_ptr<IAkPluginParam> TryParsePluginParams(FWwiseArchive& Ar, const AkPlugin& plugin,
                                                                    bool always = false)
        {
            using namespace CUE4Parse::UE4::Wwise::Plugins;

            const EAkPluginId pluginId = plugin.PluginId();
            if (pluginId == EAkPluginId::None)
                return nullptr;
            // Negative when reinterpreted as int -- C# uses that as a "not a real plugin" test.
            if (static_cast<int32_t>(pluginId) < 0 && !always)
                return nullptr;

            const uint32_t size = Ar.Read<uint32_t>();
            if (size == 0)
                return nullptr;

            const int64_t saved = Ar.Position;
            const int64_t endPosition = saved + size;
            std::unique_ptr<IAkPluginParam> params;
            try
            {
                params = Construct(Ar, pluginId, static_cast<int>(size));
            }
            catch (...)
            {
                // C# logs the exception here; the port has no logging layer.
            }
            // Always land on the declared end, whether the handler under- or over-read.
            Ar.Position = endPosition;

            return params;
        }

        static AkPlugin GetPluginId(FWwiseArchive& Ar)
        {
            const uint32_t rawId = Ar.Read<uint32_t>();
            if (rawId == UINT32_MAX || rawId == 0)
                return AkPlugin::None();

            return AkPlugin(rawId);
        }

    private:
        static std::unique_ptr<IAkPluginParam> Construct(FWwiseArchive& Ar, EAkPluginId pluginId, int size)
        {
            using namespace CUE4Parse::UE4::Wwise::Plugins;

            switch (pluginId)
            {
                // Built-in Wwise plugins
                case EAkPluginId::AkFxSrcSineSource: return std::make_unique<CAkFxSrcSineParams>(Ar);
                case EAkPluginId::AkFxSrcSilenceSource: return std::make_unique<CAkFxSrcSilenceParams>(Ar);
                case EAkPluginId::AkToneSource: return std::make_unique<CAkToneGenParams>(Ar);
                case EAkPluginId::AkParameterEQFX: return std::make_unique<CAkParameterEQFXParams>(Ar);
                case EAkPluginId::AkDelayFX: return std::make_unique<CAkDelayFXParams>(Ar);
                case EAkPluginId::AkCompressorFX: return std::make_unique<CAkCompressorFXParams>(Ar);
                case EAkPluginId::AkExpanderFX: return std::make_unique<CAkExpanderFXParams>(Ar);
                case EAkPluginId::AkPeakLimiterFX: return std::make_unique<CAkPeakLimiterFXParams>(Ar);

                case EAkPluginId::AkMatrixReverbFX: return std::make_unique<CAkFDNReverbFXParams>(Ar);
                case EAkPluginId::AkSoundSeedImpactFX: return std::make_unique<CAkModalSynthParams>(Ar);
                case EAkPluginId::AkRoomVerbFX: return std::make_unique<CAkRoomVerbFXParams>(Ar);
                case EAkPluginId::AkSoundSeedWind: return std::make_unique<CAkSoundSeedWindParams>(Ar);
                case EAkPluginId::AkSoundSeedWoosh: return std::make_unique<CAkSoundSeedWooshParams>(Ar);
                case EAkPluginId::AkFlangerFX: return std::make_unique<CAkFlangerFXParams>(Ar);
                case EAkPluginId::AkGuitarDistortionFX: return std::make_unique<CAkGuitarDistortionFXParams>(Ar);
                case EAkPluginId::AkConvolutionReverbFX: return std::make_unique<CAkConvolutionReverbFXParams>(Ar);

                case EAkPluginId::AkMeterFX: return std::make_unique<CAkMeterFXParams>(Ar);
                case EAkPluginId::AkTimeStretchFX: return std::make_unique<CAkTimeStretchFXParams>(Ar);
                case EAkPluginId::AkTremoloFX: return std::make_unique<CAkTremoloFXParams>(Ar);
                // One of the three handlers that needs the section size, not just the archive.
                case EAkPluginId::AkRecorderFX: return std::make_unique<CAkRecorderFXParams>(Ar, size);
                case EAkPluginId::AkStereoDelayFX: return std::make_unique<CAkStereoDelayFXParams>(Ar);
                case EAkPluginId::AkPitchShifterFX: return std::make_unique<CAkPitchShifterFXParams>(Ar);
                case EAkPluginId::AkHarmonizerFX: return std::make_unique<CAkHarmonizerFXParams>(Ar);
                case EAkPluginId::AkGainFX: return std::make_unique<CAkGainFXParams>(Ar);

                case EAkPluginId::AkSynthOne: return std::make_unique<CAkSynthOneParams>(Ar);

                case EAkPluginId::ASIOSink: return std::make_unique<CAkAsioSinkParams>(Ar);
                case EAkPluginId::MicrosoftHRTFSink: return std::make_unique<CMicrosoftHRTFSinkParams>(Ar);
                case EAkPluginId::AkReflectFX: return std::make_unique<CAkReflectFXParams>(Ar);
                // EAkPluginId::AkRouterMixer

                case EAkPluginId::SystemSink: return std::make_unique<CAkSystemSinkParams>(Ar);
                case EAkPluginId::DVRByPassSink: return std::make_unique<CAkDVRSinkParams>(Ar);
                case EAkPluginId::CommunicationSink:
                case EAkPluginId::ControllerHeadphonesSink:
                case EAkPluginId::VoiceSink:
                case EAkPluginId::ControllerSpeakerSink:
                case EAkPluginId::AuxiliarySink:
                case EAkPluginId::NoOutputSink:
                case EAkPluginId::RemoteSystemSink: return std::make_unique<CAkDefaultSinkParams>();

                case EAkPluginId::AkSoundSeedGrain: return std::make_unique<CAkGranularSynthParams>(Ar);
                case EAkPluginId::AkImpacterSource: return std::make_unique<CAkImpacterParams>(Ar);
                case EAkPluginId::MasteringSuiteFX: return std::make_unique<MasteringSuite::CMasteringSuiteFXParams>(Ar);
                case EAkPluginId::Ak3DAudioBedMixerFX: return std::make_unique<CAk3DAudioBedMixerFXParams>(Ar);
                case EAkPluginId::AkChannelRouterFX: return std::make_unique<CAkChannelRouterFXParams>(Ar);

                case EAkPluginId::AkSidechainSendFX: return std::make_unique<CAkSidechainSendFXParams>(Ar);
                case EAkPluginId::AkSidechainRecvFX: return std::make_unique<CAkSidechainRecvFXParams>(Ar);
                case EAkPluginId::AkMultibandMeterFX: return std::make_unique<CAkMultibandMeterFXParams>(Ar);
                case EAkPluginId::AkRecorder_ADM: return std::make_unique<CAkRecorderADMFXParams>(Ar);
                case EAkPluginId::AkAudioInputSource: return std::make_unique<CAkFxSrcAudioInputParams>(Ar);
                case EAkPluginId::ASIOSource: return std::make_unique<CAkAsioSourceParams>(Ar);

                case EAkPluginId::AkMotionGeneratorSource:
                case EAkPluginId::AkMotionGeneratorMotionSource: return std::make_unique<CAkMotionGeneratorParams>(Ar);
                case EAkPluginId::AkMotionSourceSource:
                case EAkPluginId::AkMotionSource: return std::make_unique<CAkMotionSourceParams>(Ar);
                case EAkPluginId::AkMotionSink: return std::make_unique<CAkDefaultSinkParams>();

                case EAkPluginId::AkSystemOutputMeta: return std::make_unique<CAkSystemOutputParams>(Ar);
                case EAkPluginId::AkChannelRouterMeta: return std::make_unique<CAkChannelRouterMetaParams>(Ar);

                case EAkPluginId::atmokyEars: return std::make_unique<atmoky::CAtmokyEarsFXParams>(Ar);

                // EAkPluginId::AudioSpectrumFX
                case EAkPluginId::AuroHeadphoneFX: return std::make_unique<Auro::CAuroHPFXParams>(Ar);
                case EAkPluginId::AuroPannerFX: return std::make_unique<Auro::CAuroPannerFXParams>(Ar);
                case EAkPluginId::AuroPannerMixer: return std::make_unique<Auro::CAuroPannerMixerParams>(Ar);

                // EAkPluginId::bnsRadio

                case EAkPluginId::Bitcrush: return std::make_unique<Bitcrush::CBitcrushFXParams>(Ar);
                case EAkPluginId::CrankcaseAudioREVModelPlayer:
                    return std::make_unique<CrankcaseAudioREVModelPlayer::CREVSourceModelPlayerParams>(Ar, size);

                case EAkPluginId::iZHybridReverbFX: return std::make_unique<iZotope::CiZHybridReverbFXParams>(Ar);
                case EAkPluginId::iZTrashDistortionFX: return std::make_unique<iZotope::CiZTrashDistortionFXParams>(Ar);
                case EAkPluginId::iZTrashDelayFX: return std::make_unique<iZotope::CiZTrashDelayFXParams>(Ar);
                case EAkPluginId::iZTrashDynamicsFX: return std::make_unique<iZotope::CiZTrashDynamicsFXParams>(Ar);
                case EAkPluginId::iZTrashFiltersFX: return std::make_unique<iZotope::CiZTrashFiltersFXParams>(Ar);
                case EAkPluginId::iZTrashBoxModelerFX: return std::make_unique<iZotope::CiZTrashBoxModelerFXParams>(Ar);
                case EAkPluginId::iZTrashMultibandDistortionFX:
                    return std::make_unique<iZotope::CiZTrashMultibandDistortionFXParams>(Ar);

                case EAkPluginId::AudioDataPassbackFX: return std::make_unique<Mindseye::AudioDataPassbackFXParams>(Ar);
                case EAkPluginId::BarbDelayFX: return std::make_unique<Mindseye::BarbDelayFXParams>(Ar);
                case EAkPluginId::BarbRecorderFX: return std::make_unique<Mindseye::BarbRecorderFXParams>(Ar);
                case EAkPluginId::DrunkPMSource: return std::make_unique<Mindseye::DrunkPMSourceParams>(Ar);

                case EAkPluginId::MsSpatialSink: return std::make_unique<CAkDefaultSinkParams>();

                case EAkPluginId::McDSPLimiterFX: return std::make_unique<McDSP::CMcDSPLimiterFXParams>(Ar);
                case EAkPluginId::McDSPFutzBoxFX: return std::make_unique<McDSP::CMcDSPFutzBoxFXParams>(Ar);

                case EAkPluginId::OculusAttachableMixerInputFX:
                    return std::make_unique<OculusSpatializer::COculusSpatializerFXAttachmentParams>(Ar);
                case EAkPluginId::OculusEndpointSink:
                    return std::make_unique<MetaXRAudio::OculusEndpointSinkParams>(Ar);
                case EAkPluginId::OculusEndpointMetadata:
                    return std::make_unique<MetaXRAudio::OculusEndpointMetadataParams>(Ar);
                case EAkPluginId::OculusEndpointExperimentalMetadata:
                    return std::make_unique<MetaXRAudio::OculusEndpointExperimentalMetadataParams>(Ar);
                case EAkPluginId::OculusSpatializerMixer:
                    return std::make_unique<OculusSpatializer::COculusSpatializerFXParams>(Ar);

                case EAkPluginId::PolyspectralMBC: return std::make_unique<PolyspectralMBC::CMBCRuntimeParams>(Ar, size);

                case EAkPluginId::ResonanceAudioRendererFX:
                case EAkPluginId::ResonanceAudioRoomEffectMixer:
                case EAkPluginId::ResonanceAudioRoomEffectFX:
                    return std::make_unique<ResonanceAudio::ResonanceAudioParams>(Ar);

                // EAkPluginId::IgniterLive
                // EAkPluginId::IgniterLiveSynth

                case EAkPluginId::TencentGMESendFX:
                case EAkPluginId::TencentGMESource:
                case EAkPluginId::TencentGMEReceiveSource: return std::make_unique<CAkDefaultSinkParams>();
                // EAkPluginId::TencentGMESessionFX

                default: return std::make_unique<CAkDefaultParams>(Ar, size);
            }
        }
    };
}
