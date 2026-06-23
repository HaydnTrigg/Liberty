#include "SoundCommon.h"
#include "SoundMan.h"
#include "SoundStreamer.h"

#include <windows.h>
#include <DACOM.h>

/*
 * DllMain.cpp
 *
 * Single entry point for the merged audio binary. Sound.dll contains the shared
 * audio engine (ISoundCommon), the SoundManager (3D sound effects) and the
 * SoundStreamer (streamed music). Each component provides matching
 * Register_<Name> / Shutdown_<Name> hooks (which call the DACOM
 * RegisterComponentFactory / UnregisterComponentFactory helpers); this entry
 * point simply drives them.
 */

// Marks this as a Liberty-built DLL (read by DACOM's library scan for logging).
extern "C" __declspec(dllexport) void Liberty() {}

// One-time critical-section management, implemented in SoundCommon.cpp.
extern void SoundCommon_InitLock();
extern void SoundCommon_DeleteLock();

/*
 * DLL entry point. Registers all three sound components on attach and unregisters
 * them (in reverse order) on detach.
 */
BOOL DACOM_API DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)hinstDLL;
	(void)lpvReserved;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		SoundCommon_InitLock();
		Register_SoundCommon();
		Register_SoundManager();
		Register_SoundStreamer();
		break;

	case DLL_PROCESS_DETACH:
		Shutdown_SoundStreamer();
		Shutdown_SoundManager();
		Shutdown_SoundCommon();
		SoundCommon_DeleteLock();
		break;
	}

	return TRUE;
}
