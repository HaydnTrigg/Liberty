#pragma once

#ifndef __SOUNDSTREAMER_H__
#define __SOUNDSTREAMER_H__

#include "Streamer.h"

#define CLSID_SoundStreamer "SoundStreamer"
struct DACOM_NO_VTABLE SoundStreamer : IStreamer2, IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(SoundStreamer)
		DACOM_INTERFACE_ENTRY(IStreamer)
		DACOM_INTERFACE_ENTRY(IStreamer2)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_IStreamer, IStreamer)
		DACOM_INTERFACE_ENTRY2(IID_IStreamer2, IStreamer2)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent, IAggregateComponent)
		END_DACOM_MAP()

	struct IDirectSound* lpDSound;
	HWND hMainWindow;
	UINT uMsg;
	DWORD unknown14;
	float SoundBufferTime;
	BYTE unknown1C;
	CRITICAL_SECTION criticalSection;
	BYTE unknown38;
	void* unknown3C;
	void* unknown40;
	DWORD unknown44;
	BYTE unknown48_struct;
	DWORD unknown4C;
	DWORD unknown50;
	DWORD unknown54;
	DWORD unknown58;
	DWORD unknown5C;
	HANDLE hThread;
	DWORD threadStatus;
	HANDLE hEvent;

	SoundStreamer();
	~SoundStreamer();

	DACOM_DEFMETHOD_(BOOL32, Init) (STREAMERDESC* desc) override;
	DACOM_DEFMETHOD_(HSTREAM, Open) (const char* filename, IFileSystem* parent, DWORD flags = STRMFL_PLAY) override;
	DACOM_DEFMETHOD_(BOOL32, CloseHandle) (HSTREAM hStream) override;
	DACOM_DEFMETHOD_(BOOL32, Stop) (HSTREAM hStream) override;
	DACOM_DEFMETHOD_(BOOL32, Restart) (HSTREAM hStream) override;
	DACOM_DEFMETHOD_(BOOL32, SetVolume) (HSTREAM hStream, S32 volume) override;
	DACOM_DEFMETHOD_(BOOL32, GetVolume) (HSTREAM hStream, S32* volume) const override;
	DACOM_DEFMETHOD_(STATUS, GetStatus) (HSTREAM hStream) const override;

	DACOM_DEFMETHOD_(DWORD, GetSomethingA) () override;
	DACOM_DEFMETHOD_(DWORD, GetSomethingB) () override;
	DACOM_DEFMETHOD_(BOOL32, SetPan) (HSTREAM hStream, S32 pan) override;
	DACOM_DEFMETHOD_(BOOL32, GetPan) (HSTREAM hStream, S32* pan) const override;

	DACOM_DEFMETHOD(Initialize) (void) override;

	GENRESULT init(AGGDESC* lpDesc);

	int main(void);
};

struct IDirectSoundBuffer;

struct Streamer
{
	DWORD unknown0;
	void* unknown4;
	DWORD unknown8;
	DWORD unknownC;
	DWORD unknown10;
	DWORD unknown14;
	DWORD unknown18;
	DWORD unknown1C;
	DWORD unknown20;
	DWORD unknown24;
	DWORD unknown28;
	char unknown2C[84];
	DWORD unknown80_state_flags;
	BYTE unknown84;
	BYTE unknown85;
	BYTE unknown86;
	BYTE unknown87;
	LONG current_volume;
	LONG current_pan;
	DWORD unknown90;
	DWORD unknown94;
	DWORD unknown98;
	DWORD unknown9C;
	DWORD unknownA0;
	DWORD unknownA4;
	DWORD unknownA8;
	DWORD unknownAC;
	DWORD unknownB0;
	DWORD unknownB4;
	IDirectSoundBuffer* dsound_buffer;
	DWORD unknownBC;
	DWORD unknownC0;
	DWORD unknownC4;
	DWORD unknownC8;
	DWORD unknownCC;
	DWORD unknownD0;
	DWORD unknownD4;
	DWORD unknownD8;
	DWORD unknownDC;
	DWORD unknownE0;
	DWORD unknownE4;
	DWORD unknownE8;
	DWORD unknownEC;
	DWORD unknownF0;
	DWORD unknownF4;
	DWORD unknownF8;
	DWORD unknownFC;
	DWORD unknown100;
	DWORD unknown104;
	DWORD unknown108;
	DWORD unknown10C;

	void SetPan(LONG pan);
	S32 GetPan() const;

	void SetVolume(LONG volume);
	S32 GetVolume() const;
};

extern "C"
{
	SOUNDSTREAMER_DEC IComponentFactory* CreateSoundStreamerFactory(void);
	SOUNDSTREAMER_DEC void Register_SoundStreamer();
	SOUNDSTREAMER_DEC void Shutdown_SoundStreamer();
}

#endif // __SOUNDSTREAMER_H__
