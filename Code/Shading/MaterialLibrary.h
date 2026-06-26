#pragma once

#ifndef MATERIALLIBRARY_H
#define MATERIALLIBRARY_H

#include "IMaterialLibrary.h"

/*
 * MaterialLibrary.h
 *
 * The DACOM "MaterialLibrary" component, implementing IMaterialLibrary. A plain
 * DACOM component (the IDAComponent / aggregation plumbing is supplied by the
 * DAComponentAggregate<> wrapper); the registration hooks are the hand-written
 * Register_/Shutdown_MaterialLibrary in MaterialLibrary.cpp.
 */
#define CLSID_MaterialLibrary "MaterialLibrary"
struct MaterialLibrary : public IMaterialLibrary
{
	BEGIN_DACOM_MAP_INBOUND(MaterialLibrary)
		DACOM_INTERFACE_ENTRY2(IID_IMaterialLibrary, IMaterialLibrary)
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
	SHADING_DEC void Register_MaterialLibrary();
	SHADING_DEC void Shutdown_MaterialLibrary();
}

#endif // MATERIALLIBRARY_H
