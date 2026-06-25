#pragma once

#ifndef ISYSTEM_H
#define ISYSTEM_H

#include <DACOM.h>

/*
 * ISystem.h
 *
 * Public DACOM interfaces implemented by system.dll's SystemContainer: the
 * container itself and the ISystemComponent contract its children honour.
 *
 * Ported from Conquest's <system.h> and reconciled against the shipped
 * Freelancer system.dll (system.dll.i64). The original Conquest ISystemComponent
 * also declared a ShutdownAggregate() method, but the shipped binary has no such
 * vtable slot (the SystemContainer vtable is exactly eight entries and Shutdown()
 * simply releases each element), so it is omitted here.
 */

/*
 * A component that wants per-frame Update() ticks from its container. Derives
 * from IAggregateComponent so the container can also drive Initialize().
 */
#define IID_ISystemComponent DACOM_MAKE_IID("ISystemComponent")
DACOM_INTERFACE(ISystemComponent, IID_ISystemComponent);

struct DACOM_NO_VTABLE ISystemComponent : public IAggregateComponent
{
	DACOM_DEFMETHOD_(void, Update) (void) = 0;
};

/*
 * The top-level container built from the "[System]" profile section. Create an
 * instance with an AGGDESC whose interface_name is "SystemContainer".
 */
#define IID_ISystemContainer DACOM_MAKE_IID("ISystemContainer")
DACOM_INTERFACE(ISystemContainer, IID_ISystemContainer);

struct DACOM_NO_VTABLE ISystemContainer : public ISystemComponent
{
	DACOM_DEFMETHOD(LoadSystemComponents) (void) = 0;
	DACOM_DEFMETHOD(Shutdown) (void) = 0;
	DACOM_DEFMETHOD(AddComponent) (const AGGDESC* descriptor) = 0;
};

/*
 * Component registration hooks, invoked from system.dll's entry point
 * (DllMain.cpp). SYSTEM_DEC is the host module's export/import decoration.
 */
#ifndef SYSTEM_DEC
#define SYSTEM_DEC
#endif

extern "C"
{
	SYSTEM_DEC void Register_SystemContainer();
	SYSTEM_DEC void Shutdown_SystemContainer();
}

#endif // ISYSTEM_H
