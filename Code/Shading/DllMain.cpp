#include <Windows.h>
#include <DACOM.h>
#include <PCH.h>
#include "Material.h"
#include "TextureLibrary.h"
#include "MaterialLibrary.h"
#include "MaterialBatcher.h"
#include "MaterialAnimation.h"

/*
 * DllMain.cpp
 *
 * Entry point for shading.dll. The DLL hosts the material DACOM components;
 * this entry point registers them all on attach (REGISTER_MATERIAL) and
 * unregisters them on detach (SHUTDOWN_MATERIAL). The Register_/Shutdown_
 * hooks are emitted by DECLARE_MATERIAL in each component's .cpp.
 */

// Marks this as a Liberty-built DLL (read by DACOM's library scan for logging).
extern "C" __declspec(dllexport) void Liberty() {}

_naked BOOL __stdcall EntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	asm("movl ___entry_ptr, %eax");
	asm("jmp *%eax");
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	BOOL Result = EntryPoint(hinstDLL, fdwReason, lpvReserved);
	(void)lpvReserved;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);

		Register_TextureLibrary();
		break;

	case DLL_PROCESS_DETACH:
		Shutdown_TextureLibrary();
		break;
	}
	return Result;
}


#if 0
BOOL DACOM_API DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)lpvReserved;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);

		Register_TextureLibrary();
		Register_MaterialLibrary();
		Register_MaterialBatcher();
		Register_MaterialAnimation();
		REGISTER_MATERIAL(Material);
		REGISTER_MATERIAL(SinglePassMaterial);

		REGISTER_MATERIAL(DcDtEcOcOtMaterial);
		REGISTER_MATERIAL(DcDtEcOcOtTwoMaterial);
		REGISTER_MATERIAL(DcDtEcMaterial);
		REGISTER_MATERIAL(DcDtEcTwoMaterial);
		REGISTER_MATERIAL(DcDtEtMaterial);
		REGISTER_MATERIAL(DcDtMaterial);
		REGISTER_MATERIAL(DcDtTwoMaterial);
		REGISTER_MATERIAL(DcDtOcOtMaterial);
		REGISTER_MATERIAL(DcDtOcOtTwoMaterial);
		REGISTER_MATERIAL(EcEtMaterial);
		REGISTER_MATERIAL(EcEtTwoMaterial);
		REGISTER_MATERIAL(AtmosphereMaterial);
		REGISTER_MATERIAL(NebulaMaterial);
		REGISTER_MATERIAL(NebulaTwoMaterial);
		REGISTER_MATERIAL(NullMaterial);
		REGISTER_MATERIAL(LightMapMaterial);
		REGISTER_MATERIAL(DetailMapMaterial);
		REGISTER_MATERIAL(DetailMap2Dm1Msk2PassMaterial);
		REGISTER_MATERIAL(Masked2DetailMapMaterial);
		REGISTER_MATERIAL(IllumDetailMapMaterial);
		REGISTER_MATERIAL(BtDetailMapMaterial);
		REGISTER_MATERIAL(BtDetailMapTwoMaterial);
		break;

	case DLL_PROCESS_DETACH:
		Shutdown_TextureLibrary();
		Shutdown_MaterialLibrary();
		Shutdown_MaterialBatcher();
		Shutdown_MaterialAnimation();
		SHUTDOWN_MATERIAL(Material);

		SHUTDOWN_MATERIAL(DcDtEcOcOtMaterial);
		SHUTDOWN_MATERIAL(DcDtEcOcOtTwoMaterial);
		SHUTDOWN_MATERIAL(DcDtEcMaterial);
		SHUTDOWN_MATERIAL(DcDtEcTwoMaterial);
		SHUTDOWN_MATERIAL(DcDtEtMaterial);
		SHUTDOWN_MATERIAL(DcDtMaterial);
		SHUTDOWN_MATERIAL(DcDtTwoMaterial);
		SHUTDOWN_MATERIAL(DcDtOcOtMaterial);
		SHUTDOWN_MATERIAL(DcDtOcOtTwoMaterial);
		SHUTDOWN_MATERIAL(EcEtMaterial);
		SHUTDOWN_MATERIAL(EcEtTwoMaterial);
		SHUTDOWN_MATERIAL(AtmosphereMaterial);
		SHUTDOWN_MATERIAL(NebulaMaterial);
		SHUTDOWN_MATERIAL(NebulaTwoMaterial);
		SHUTDOWN_MATERIAL(NullMaterial);
		SHUTDOWN_MATERIAL(LightMapMaterial);
		SHUTDOWN_MATERIAL(DetailMapMaterial);
		SHUTDOWN_MATERIAL(DetailMap2Dm1Msk2PassMaterial);
		SHUTDOWN_MATERIAL(Masked2DetailMapMaterial);
		SHUTDOWN_MATERIAL(IllumDetailMapMaterial);
		SHUTDOWN_MATERIAL(BtDetailMapMaterial);
		SHUTDOWN_MATERIAL(BtDetailMapTwoMaterial);
		break;
	}

	return TRUE;
}
#endif