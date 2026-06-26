#pragma once

#ifndef ICONNECTION_H
#define ICONNECTION_H

#include <DACOM.h>

/*
 * IConnection.h
 *
 * Connection-point interfaces (a lightweight COM-connection-point analogue).
 * SystemContainer implements IDAConnectionPointContainer and forwards
 * FindConnectionPoint / EnumerateConnectionPoints to its child components.
 *
 * Ported from Conquest's <IConnection.h>; the trailing inline
 * IDAComponent::QueryOutgoingInterface helper from the original header is omitted
 * (Liberty's IDAComponent does not declare that method).
 */

struct IDAConnectionPoint;
struct IDAConnectionPointContainer;

// Enumeration callbacks - return TRUE to continue the enumeration.
typedef BOOL32(__stdcall* CONNECTION_ENUM_PROC) (struct IDAConnectionPoint* connPoint, struct IDAComponent* client, void* context);
typedef BOOL32(__stdcall* CONNCONTAINER_ENUM_PROC) (struct IDAConnectionPointContainer* container, struct IDAConnectionPoint* connPoint, void* context);

/*
 * A single outgoing connection point: a named outgoing interface that clients can
 * Advise() to receive callbacks on.
 */
#define IID_IDAConnectionPoint DACOM_MAKE_IID("IDAConnectionPoint")
DACOM_INTERFACE(IDAConnectionPoint, IID_IDAConnectionPoint);

struct DACOM_NO_VTABLE IDAConnectionPoint : public IDAComponent
{
	DEFMETHOD_(U32, GetOutgoingInterface) (C8* interfaceName, U32 bufferLength) = 0;
	DEFMETHOD(GetContainer) (IDAConnectionPointContainer** container) = 0;
	DEFMETHOD(Advise) (IDAComponent* component, U32* handle) = 0;
	DEFMETHOD(Unadvise) (U32 handle) = 0;
	DEFMETHOD_(BOOL32, EnumerateConnections) (CONNECTION_ENUM_PROC proc, void* context = 0) = 0;
};

/*
 * A component that owns one or more connection points and can look them up or
 * enumerate them by name.
 */
#define IID_IDAConnectionPointContainer DACOM_MAKE_IID("IDAConnectionPointContainer")
DACOM_INTERFACE(IDAConnectionPointContainer, IID_IDAConnectionPointContainer);

struct DACOM_NO_VTABLE IDAConnectionPointContainer : public IDAComponent
{
	DEFMETHOD(FindConnectionPoint) (const C8* connectionName, struct IDAConnectionPoint** connPoint) = 0;
	DEFMETHOD_(BOOL32, EnumerateConnectionPoints) (CONNCONTAINER_ENUM_PROC proc, void* context = 0) = 0;
};

#endif // ICONNECTION_H
