#pragma once

//--------------------------------------------------------------------------//
//---------------------------ICOManager Interface---------------------------//
//--------------------------------------------------------------------------//

//-------------------------------------
// Priorities for RegisterComponent
//-------------------------------------
#define DACOM_HIGH_PRIORITY 0xC0000000
#define DACOM_NORMAL_PRIORITY 0x80000000
#define DACOM_LOW_PRIORITY 0x40000000

//-------------------------------------
// callback prototype for EnumerateComponents
//-------------------------------------
typedef BOOL32(__cdecl* DACOMENUMCALLBACK)(IComponentFactory* component,
	const C8* interface_name,
	U32 priority,
	void* context);

//-------------------------------------
// flags for SetINIConfig
//-------------------------------------
#define DACOM_INI_WRITABLE	0x00000001
#define DACOM_INI_STRING	0x00000002

#define IID_ICOManager DACOM_MAKE_IID("ICOManager")
DACOM_INTERFACE(ICOManager, IID_ICOManager);
struct DACOM_NO_VTABLE ICOManager : public IComponentFactory
{
	DEFMETHOD(RegisterComponent)	 (IComponentFactory* component, const C8* interface_name, U32 priority = DACOM_NORMAL_PRIORITY) = 0;
	DEFMETHOD(UnregisterComponent) (IComponentFactory* component, const C8* interface_name = 0) = 0;
	DEFMETHOD(EnumerateComponents) (const C8* interface_name, DACOMENUMCALLBACK callback, void* context = 0) = 0;
	DEFMETHOD(AddLibrary) (const C8* DLL_filename) = 0;
	DEFMETHOD(RemoveLibrary) (const C8* DLL_filename) = 0;
	DEFMETHOD(ShutDown) (void) = 0;
	DEFMETHOD(SetINIConfig) (const C8* info, U32 flags = 0) = 0;
};

// Extensions for peeking into the internals of ICOManager
#define IID_ICOManagerLiberty DACOM_MAKE_IID("ICOManagerLiberty")
DACOM_INTERFACE(ICOManagerLiberty, IID_ICOManagerLiberty);
struct DACOM_NO_VTABLE ICOManagerLiberty
{
	DEFMETHOD(CreateInstanceEx) (DACOMDESC* descriptor, void** instance, U32 low_priority, U32 high_priority) = 0;
};
