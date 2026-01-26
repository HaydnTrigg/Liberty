#pragma once

#ifndef __STREAMER_H__
#define __STREAMER_H__

#include <DACOM.h>

typedef struct Streamer* HSTREAM;

struct STREAMERDESC
{
	U32 size;
	struct IDirectSound* lpDSound;
	HWND hMainWindow;
	UINT uMsg;
	SINGLE readBufferTime;		// in seconds, 0 = default setting (4.0 is a reasonable value)
	SINGLE soundBufferTime;		// in seconds, 0 = default setting (0.25 is a reasonable value)

	STREAMERDESC(void) :
		size(sizeof(*this)),
		lpDSound(nullptr),
		hMainWindow(NULL),
		uMsg(),
		readBufferTime(),
		soundBufferTime()
	{
	}
};

#define STRMFL_PLAY			0x00000001			// start stream once opened
#define STRMFL_LOOPING		0x00000002			// loop the entire stream

#define IID_IStreamer DACOM_MAKE_IID("IStreamer")
#define IID_IStreamer2 DACOM_MAKE_IID("IStreamer2")

struct DACOM_NO_VTABLE IStreamer : IDAComponent
{
	enum STATUS
	{
		INVALID = 0,
		PLAYING,
		STOPPED,
		EOFREACHED,
		COMPLETED,
		INITSUCCESS
	};

	DACOM_DEFMETHOD_(BOOL32, Init) (STREAMERDESC* desc) = 0;
	DACOM_DEFMETHOD_(HSTREAM, Open) (const char* filename, struct IFileSystem* parent, DWORD flags = STRMFL_PLAY) = 0;
	DACOM_DEFMETHOD_(BOOL32, CloseHandle) (HSTREAM hStream) = 0;
	DACOM_DEFMETHOD_(BOOL32, Stop) (HSTREAM hStream) = 0;
	DACOM_DEFMETHOD_(BOOL32, Restart) (HSTREAM hStream) = 0;
	DACOM_DEFMETHOD_(BOOL32, SetVolume) (HSTREAM hStream, S32 volume) = 0;
	DACOM_DEFMETHOD_(BOOL32, GetVolume) (HSTREAM hStream, S32* volume) const = 0;
	DACOM_DEFMETHOD_(STATUS, GetStatus) (HSTREAM hStream) const = 0;
};

struct DACOM_NO_VTABLE IStreamer2 : IStreamer
{
	DACOM_DEFMETHOD_(DWORD, GetSomethingA) () = 0;
	DACOM_DEFMETHOD_(DWORD, GetSomethingB) () = 0;
	DACOM_DEFMETHOD_(BOOL32, SetPan) (HSTREAM hStream, S32 pan) = 0;
	DACOM_DEFMETHOD_(BOOL32, GetPan) (HSTREAM hStream, S32* pan) const = 0;
};

#endif // __STREAMER_H__
