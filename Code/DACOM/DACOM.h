#pragma once

#include <Core/Core.h>

#ifndef DACOM_DEC
#define DACOM_DEC __declspec(dllimport)
#endif
#define DACOM_API __stdcall
#define DEFMETHOD(method) virtual GENRESULT DACOM_API method
#define DEFMETHOD_(type, method) virtual type DACOM_API method
#define DACOM_NO_VTABLE __declspec(novtable)
//#define DACOM_MAKE_IID(name, ver) name "__" #ver
#define DACOM_MAKE_IID(name) DA_XSTR(LIB_MAJOR) "." DA_XSTR(LIB_MINOR) "_" name

// NOTE: the clearing behavior of the new operator is necessary
// NOTE: for some components, hence let it in there.

#define DA_HEAP_DEFINE_NEW_OPERATOR(classname)	\
	void * operator new( size_t size ) { return ::calloc( size, 1 ); } \
	void operator delete( void *ptr ) { ::free( ptr ); }

#define DA_HEAP_DEFINE_NEW_OPERATOR_DECLARE_HACK(declare, classname, _calloc, _free)	\
	declare void * operator new( size_t size ); \
	declare void operator delete( void *ptr );

#define DA_HEAP_DEFINE_NEW_OPERATOR_IMPL_HACK(declare, classname, _calloc, _free)	\
	void * classname::operator new( size_t size ) { return reinterpret_cast<decltype(&::calloc)>(_calloc)( size, 1 ); } \
	void classname::operator delete( void *ptr ) { reinterpret_cast<decltype(&::free)>(_free)( ptr ); }

#include "FDump.h"
#include "IDAComponent.h"
#include "ICOManager.h"
#include "IProfileParser.h"
#include "TComponent.h"

// extern "C" interface to guarantee static linkage without name-mangling 
extern "C"
{
	// All clients of DACOM (including component objects as well as the
	// application itself) must call DACOM_Acquire() to obtain an instance
	// pointer to the DA Component Manager
	DACOM_DEC ICOManager* __cdecl DACOM_Acquire(void);

	// This allows clients to retrieve the version information about a DLL.
	// The version information is pulled out of the product version resource in the DLL and
	// returned.
	DACOM_DEC GENRESULT __cdecl DACOM_GetDllVersion(const char* dll_name, U32* out_major, U32* out_minor, U32* out_build);

	// This allows clients to retrieve DACOM information before acquiring/initializing
	// DACOM itself.  This loads the version out of the product version resource of the 
	// DACOM.dll that will be/would be/was used when the application calls DACOM_Acquire()
	DACOM_DEC GENRESULT __cdecl DACOM_GetVersion(U32* out_major, U32* out_minor, U32* out_build);
}

namespace DACOM_CRC
{
	DACOM_DEC int __cdecl CompareStringsI(char const* String1, char const* String2);
	DACOM_DEC unsigned long __cdecl GetCRC32(char const* start, char const* end);
	DACOM_DEC unsigned long __cdecl GetCRC32(char const* string);
	DACOM_DEC unsigned long __cdecl GetContinuedCRC32(unsigned long, char);
	DACOM_DEC unsigned long __cdecl GetContinuedCRC32(unsigned long, char const*);
}

namespace LogStream
{
	DACOM_DEC void FlushToDisk(void);
	DACOM_DEC void LogEvent(char const*, float, unsigned long);
	DACOM_DEC void LogNamedEvent(char const*, char const*, unsigned long);
	DACOM_DEC bool Startup(char const*);
	DACOM_DEC void Update(float);
};

//--------------------------------------------------------------------------//
//---------------------Component registration helpers-----------------------//
//--------------------------------------------------------------------------//
//
// Convenience helpers for the common "create a component factory, register it
// with the manager, then later unregister it" pattern every Liberty DLL uses.
//
// 'library_name' identifies the host module in diagnostics; each build target
// sets it through the DACOM_LIBRARY_NAME compile definition (see the CMake
// add_liberty_project macro). 'priority' is required.
//

#ifndef DACOM_LIBRARY_NAME
#define DACOM_LIBRARY_NAME "DACOM"
#endif

// Creates and registers an aggregatable component factory for 'interface_name'.
// Returns the factory pointer (kept alive by the manager) to hand back to
// UnregisterComponentFactory(); returns NULL on failure.
template <typename ClassType, typename DescType = AGGDESC>
IComponentFactory* RegisterComponentFactory(const char* library_name, const char* interface_name, U32 priority)
{
	ICOManager* pDACOM = DACOM_Acquire();
	if (pDACOM == nullptr)
	{
		GENERAL_WARNING(TEMPSTR("%s: unable to acquire DACOM to register component '%s'!\n", library_name, interface_name));
		return nullptr;
	}

	IComponentFactory* factory = new DAComponentFactory2<ClassType, DescType>(interface_name);
	if (factory == nullptr)
	{
		GENERAL_WARNING(TEMPSTR("%s: unable to create factory for '%s'!\n", library_name, interface_name));
		return nullptr;
	}

	if (pDACOM->RegisterComponent(factory, interface_name, priority) != GR_OK)
	{
		GENERAL_WARNING(TEMPSTR("%s: unable to register component '%s'!\n", library_name, interface_name));
		factory->Release();
		return nullptr;
	}

	// The manager keeps its own reference; release ours but return the pointer as a handle for UnregisterComponentFactory().
	factory->Release();
	return factory;
}

// Unregisters a factory previously returned by RegisterComponentFactory().
inline void UnregisterComponentFactory(const char* library_name, IComponentFactory* factory, const char* interface_name)
{
	ICOManager* pDACOM = DACOM_Acquire();
	if (pDACOM == nullptr)
	{
		GENERAL_WARNING(TEMPSTR("%s: unable to acquire DACOM to unregister component '%s'!\n", library_name, interface_name));
		return;
	}

	if (factory != nullptr)
	{
		pDACOM->UnregisterComponent(factory, interface_name);
	}
}
