#pragma once

#ifndef SOUNDCOMMON_H
#define SOUNDCOMMON_H

#include "ISoundCommon.h"

/*
 * SoundCommon.h
 *
 * The SoundCommon DACOM component. It implements ISoundCommon on top of a
 * process-wide, reference-counted XAudio2/X3DAudio engine (see SoundCommon.cpp).
 * All XAudio2 detail is confined to the implementation; this header and the
 * public interface stay backend-neutral.
 */

struct DACOM_NO_VTABLE SoundCommon : ISoundCommon, IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(SoundCommon)
		DACOM_INTERFACE_ENTRY(ISoundCommon)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_ISoundCommon, ISoundCommon)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent, IAggregateComponent)
		END_DACOM_MAP()

	SoundCommon() = default;
	~SoundCommon() = default;

	// *** ISoundCommon methods ***

	DACOM_DEFMETHOD_(BOOL32, Startup) () override;
	DACOM_DEFMETHOD_(void, Shutdown) () override;
	DACOM_DEFMETHOD_(U32, GetOutputChannels) () override;

	DACOM_DEFMETHOD_(HSOUNDVOICE, CreateVoice) (const WAVEFORMATEX* format) override;
	DACOM_DEFMETHOD_(void, DestroyVoice) (HSOUNDVOICE voice) override;

	DACOM_DEFMETHOD_(BOOL32, SubmitBuffer) (HSOUNDVOICE voice, const SOUND_BUFFER* buffer) override;
	DACOM_DEFMETHOD_(void, Start) (HSOUNDVOICE voice) override;
	DACOM_DEFMETHOD_(void, Stop) (HSOUNDVOICE voice) override;
	DACOM_DEFMETHOD_(void, Flush) (HSOUNDVOICE voice) override;
	DACOM_DEFMETHOD_(U32, GetBuffersQueued) (HSOUNDVOICE voice) override;

	DACOM_DEFMETHOD_(void, SetVolume) (HSOUNDVOICE voice, S32 centibels) override;
	DACOM_DEFMETHOD_(void, SetFrequencyRatio) (HSOUNDVOICE voice, SINGLE ratio) override;
	DACOM_DEFMETHOD_(void, SetPan) (HSOUNDVOICE voice, S32 panCentibels, U32 sourceChannels, SINGLE reverbSend) override;
	DACOM_DEFMETHOD_(void, Apply3D) (HSOUNDVOICE voice, const SOUND_LISTENER* listener, const SOUND_EMITTER* emitter, SINGLE frequencyRatio, SINGLE reverbSend) override;

	DACOM_DEFMETHOD_(void, SetReverb) (U32 environment, SINGLE volume, SINGLE decayScale, SINGLE damping) override;

	// *** IAggregateComponent methods ***

	DACOM_DEFMETHOD(Initialize) (void) override;

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(AGGDESC* desc);
};

/*
 * Component registration hooks, invoked from the merged binary's single entry
 * point (DllMain.cpp). SOUND_DEC is the host module's export/import decoration.
 */
extern "C"
{
	SOUND_DEC void Register_SoundCommon();
	SOUND_DEC void Shutdown_SoundCommon();
}

#endif // SOUNDCOMMON_H
