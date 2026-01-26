#include "SoundStreamer.h"
#include "NewStream.h"

#include <dsound.h>

static SoundStreamer* global_sound_streamer = NULL;

extern "C"
{
#define SoundStreamer_Vtable off_6EFC204
#define SoundStreamer_QueryInterface sub_6EF3E10
#define SoundStreamer_AddRef sub_6EF3E20
#define SoundStreamer_Release sub_6EF3E30
#define SoundStreamer_Init sub_6EF2E00
#define SoundStreamer_Open sub_6EF3060
#define SoundStreamer_CloseHandle sub_6EF3290
#define SoundStreamer_Stop sub_6EF33F0
#define SoundStreamer_Restart sub_6EF3420
#define SoundStreamer_SetVolume sub_6EF3470
#define SoundStreamer_GetVolume sub_6EF34A0
#define SoundStreamer_GetStatus sub_6EF3530
#define SoundStreamer_GetSomethingA sub_6EF3940
#define SoundStreamer_GetSomethingB sub_6EF3950
#define SoundStreamer_SetPan sub_6EF34D0
#define SoundStreamer_GetSomethingC sub_6EF3500
#define SoundStreamer_Ctor sub_6EF2AF0
#define SoundStreamer_Initialize sub_6EF2DB0
#define SoundStreamer_init sub_6EF2D80
#define SoundStreamer_main sub_6EF35B0

#pragma comment(linker, "/alternatename:_sub_6EF2E00@8=_sub_6EF2E00") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3060@16=_sub_6EF3060") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3290@8=_sub_6EF3290") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF33F0@8=_sub_6EF33F0") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3420@8=_sub_6EF3420") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3470@12=_sub_6EF3470") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF34A0@12=_sub_6EF34A0") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3530@8=_sub_6EF3530") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3940@4=_sub_6EF3940") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3950@4=_sub_6EF3950") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF34D0@12=_sub_6EF34D0") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF3500@12=_sub_6EF3500") // stdcall
#pragma comment(linker, "/alternatename:_sub_6EF2DB0@4=_sub_6EF2DB0") // stdcall

	GENRESULT __stdcall SoundStreamer_QueryInterface(SoundStreamer* _this, const C8* interface_name, void** instance);
	U32 __stdcall SoundStreamer_AddRef(SoundStreamer* _this);
	U32 __stdcall SoundStreamer_Release(SoundStreamer* _this);
	BOOL32 __stdcall SoundStreamer_Init(SoundStreamer* _this, STREAMERDESC* desc);
	HSTREAM __stdcall SoundStreamer_Open(SoundStreamer* _this, const char* filename, struct IFileSystem* parent, DWORD flags);
	BOOL32 __stdcall SoundStreamer_CloseHandle(SoundStreamer* _this, HSTREAM hStream);
	BOOL32 __stdcall SoundStreamer_Stop(SoundStreamer* _this, HSTREAM hStream);
	BOOL32 __stdcall SoundStreamer_Restart(SoundStreamer* _this, HSTREAM hStream);
	BOOL32 __stdcall SoundStreamer_SetVolume(SoundStreamer* _this, HSTREAM hStream, S32 volume);
	BOOL32 __stdcall SoundStreamer_GetVolume(SoundStreamer const* _this, HSTREAM hStream, S32* volume);
	IStreamer::STATUS __stdcall SoundStreamer_GetStatus(SoundStreamer const* _this, HSTREAM hStream);
	DWORD __stdcall SoundStreamer_GetSomethingA(SoundStreamer const* _this);
	DWORD __stdcall SoundStreamer_GetSomethingB(SoundStreamer const* _this);
	BOOL32 __stdcall SoundStreamer_SetPan(SoundStreamer const* _this, HSTREAM hStream, DWORD arg);
	BOOL32 __stdcall SoundStreamer_GetSomethingC(SoundStreamer const* _this, HSTREAM hStream, DWORD* arg);
	SoundStreamer* __thiscall SoundStreamer_Ctor(SoundStreamer* _this);
	GENRESULT __stdcall SoundStreamer_Initialize(void* _this);
	GENRESULT __thiscall SoundStreamer_init(void* _this, AGGDESC* lpDesc);
	int __thiscall SoundStreamer_main(void* _this);
}

#define _Shutdown_SoundStreamer sub_6EF2BB0
extern "C" void _Shutdown_SoundStreamer(SoundStreamer* pSoundStreamer);

extern "C"
{
	IComponentFactory* CreateSoundStreamerFactory(void)
	{
		return new DAComponentFactory2<DAComponentAggregate<SoundStreamer>, AGGDESC>(CLSID_SoundStreamer);
	}

	void Register_SoundStreamer()
	{
		if (IComponentFactory* lpSoundStreamer = CreateSoundStreamerFactory())
		{
			ICOManager* DACOM = DACOM_Acquire();
			DACOM->RegisterComponent(lpSoundStreamer, CLSID_SoundStreamer, DACOM_LOW_PRIORITY);
			lpSoundStreamer->Release();
		}
	}

	void Shutdown_SoundStreamer()
	{
		if (global_sound_streamer)
		{
			_Shutdown_SoundStreamer(global_sound_streamer);
		}
	}
}

GENRESULT SoundStreamer::Initialize(void)
{
	this->lpDSound = 0;
	this->hMainWindow = 0;
	this->uMsg = 0;
	this->unknown14 = 0;
	this->SoundBufferTime = 0.0;
	this->hThread = 0;
	this->threadStatus = 0;
	InitAcmDriver();
	return GR_OK;
}

DWORD WINAPI threadMain(LPVOID lpThreadParameter)
{
	HRESULT result = 0;
	if (SUCCEEDED(result = CoInitialize(NULL)))
	{
		SoundStreamer* pSoundStreamer = reinterpret_cast<SoundStreamer*>(lpThreadParameter);
		result = pSoundStreamer->main();

		CoUninitialize();
	}
	return static_cast<DWORD>(result);
}

BOOL32 SoundStreamer::Init(STREAMERDESC* desc)
{
	if ((desc->size != sizeof(*desc)))
	{
		GENERAL_ERROR("SoundStreamer - Invalid parameters.");
		return FALSE;
	}

	hMainWindow = desc->hMainWindow;
	uMsg = desc->uMsg;
	if (desc->lpDSound)
	{
		lpDSound = desc->lpDSound;
		lpDSound->AddRef();
	}
	else
	{
		NOT_IMPLEMENTED;
	}

	SoundBufferTime = desc->soundBufferTime;
	if (SoundBufferTime == 0.0)
		SoundBufferTime = 1.0;
	hThread = CreateThread(0, 0x1000u, &threadMain, this, 0, NULL);



	if (hThread == 0)
	{
		desc->lpDSound->Release();
		this->lpDSound = NULL;
		return 0;
	}

	while ((threadStatus & 3) == 0)		// wait for thread to start
		;

	BOOL32 result = 1;

	return result;
}

HSTREAM SoundStreamer::Open(const char* filename, IFileSystem* parent, DWORD flags)
{
	HSTREAM result = SoundStreamer_Open(this, filename, parent, flags);
	return result;
}

BOOL32 SoundStreamer::CloseHandle(HSTREAM hStream)
{
	BOOL32 result = SoundStreamer_CloseHandle(this, hStream);
	return result;
}

BOOL32 SoundStreamer::Stop(HSTREAM hStream)
{
	BOOL32 result = SoundStreamer_Stop(this, hStream);
	return result;
}

BOOL32 SoundStreamer::Restart(HSTREAM hStream)
{
	BOOL32 result = SoundStreamer_Restart(this, hStream);
	return result;
}

BOOL32 SoundStreamer::SetVolume(HSTREAM hStream, S32 volume)
{
	BOOL32 result = false;
	if (hStream != nullptr && hThread != NULL)
	{
		hStream->SetVolume(volume);
		result = true;
	}
	return result;
}

BOOL32 SoundStreamer::GetVolume(HSTREAM hStream, S32* volume) const
{
	BOOL32 result = false;
	if (hStream != nullptr && hThread != NULL)
	{
		*volume = hStream->GetVolume();
		result = true;
	}
	return result;
}

SoundStreamer::STATUS SoundStreamer::GetStatus(HSTREAM hStream) const
{
	SoundStreamer::STATUS result = SoundStreamer_GetStatus(this, hStream);
	return result;
}

DWORD SoundStreamer::GetSomethingA()
{
	NOT_IMPLEMENTED;
	return unknown58;
}

DWORD SoundStreamer::GetSomethingB()
{
	NOT_IMPLEMENTED;
	return unknown5C;
}

BOOL32 SoundStreamer::SetPan(HSTREAM hStream, S32 pan)
{
	BOOL32 result = false;
	if (hStream != nullptr && hThread != NULL)
	{
		hStream->SetPan(pan);
		result = true;
	}
	return result;
}

BOOL32 SoundStreamer::GetPan(HSTREAM hStream, S32* pan) const
{
	BOOL32 result = false;
	if (hStream != nullptr && hThread != NULL)
	{
		*pan = hStream->GetPan();
		result = true;
	}
	return result;
}

SoundStreamer::SoundStreamer()
{
	this->unknown1C = 0;
	InitializeCriticalSection(&this->criticalSection);
	this->unknown38 = 0;
	this->unknown3C = 0;
	this->unknown40 = 0;
	this->unknown44 = 0;
	this->unknown48_struct = 0;
	this->unknown4C = 0;
	this->unknown50 = 0;
	this->unknown54 = 0;
	this->hEvent = CreateEventA(0, 0, 0, 0);
	this->unknown58 = 0;
	this->unknown5C = 0;

	//SoundStreamer_Ctor(this);
}

SoundStreamer::~SoundStreamer()
{
	// does this even do anything??????
	// NOT_IMPLEMENTED;
}

GENRESULT SoundStreamer::init(AGGDESC* lpDesc)
{
	if (global_sound_streamer == nullptr)
	{
		global_sound_streamer = this;
		Initialize();
		return GR_OK;
	}
	else
	{
		return GR_GENERIC;
	}
}

int SoundStreamer::main()
{
	int result = SoundStreamer_main(this);
	return result;
	//this->threadStatus |= 1u;                     // set active flag
	//while ((threadStatus & 0x80) == 0)
	//{
	//	Sleep(1);
	//}  
	//this->threadStatus |= 2u;
	//return 1;
}

void Streamer::SetPan(LONG pan)
{
	current_pan = pan;
	if (dsound_buffer)
	{
		dsound_buffer->SetPan(pan);
	}
}

S32 Streamer::GetPan() const
{
	LONG pan;
	if (dsound_buffer && SUCCEEDED(dsound_buffer->GetPan(&pan)))
	{
		return pan;
	}

	return current_pan;
}

void Streamer::SetVolume(LONG volume)
{
	current_volume = volume;
	if (dsound_buffer)
	{
		dsound_buffer->SetVolume(volume);
	}
}

S32 Streamer::GetVolume() const
{
	LONG volume;
	if (dsound_buffer && SUCCEEDED(dsound_buffer->GetVolume(&volume)))
	{
		return volume;
	}

	return current_volume;
}
