#pragma once

#ifndef MATERIALBATCHER_H
#define MATERIALBATCHER_H

#include "IMaterialBatcher.h"

/*
 * MaterialBatcher.h
 *
 * The DACOM "MaterialBatcher" component, implementing IMaterialBatcher. A plain
 * DACOM component (the IDAComponent / aggregation plumbing is supplied by the
 * DAComponentAggregate<> wrapper); the registration hooks are the hand-written
 * Register_/Shutdown_MaterialBatcher in MaterialBatcher.cpp.
 */
#define CLSID_MaterialBatcher "MaterialBatcher"
struct MaterialBatcher : public IMaterialBatcher
{
	BEGIN_DACOM_MAP_INBOUND(MaterialBatcher)
		DACOM_INTERFACE_ENTRY2(IID_IMaterialBatcher, IMaterialBatcher)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(AGGDESC* info);
};

/*
 * Component registration hooks, invoked from shading.dll's entry point
 * (DllMain.cpp). SHADING_DEC is the host module's export/import decoration.
 */
extern "C"
{
	SHADING_DEC void Register_MaterialBatcher();
	SHADING_DEC void Shutdown_MaterialBatcher();
}

#endif // MATERIALBATCHER_H
