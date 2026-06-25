#include <windows.h>
#include <DACOM.h>

#include "ISystem.h"

/*
 * DllMain.cpp
 *
 * Entry point for system.dll. The DLL contains a single DACOM component, the
 * SystemContainer (the "[System]" section loader); this entry point registers it
 * on attach and unregisters it on detach.
 *
 * The shipped system.dll only registered on attach (it did no detach cleanup);
 * the symmetric Shutdown_SystemContainer() here follows the modern Liberty
 * convention and is harmless to the original behaviour.
 */

// Marks this as a Liberty-built DLL (read by DACOM's library scan for logging).
extern "C" __declspec(dllexport) void Liberty() {}

BOOL DACOM_API DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)lpvReserved;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		Register_SystemContainer();
		break;

	case DLL_PROCESS_DETACH:
		Shutdown_SystemContainer();
		break;
	}

	return TRUE;
}
