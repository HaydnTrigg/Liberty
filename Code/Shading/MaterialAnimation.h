#pragma once

#ifndef MATERIALANIMATION_H
#define MATERIALANIMATION_H

#include "IMaterialAnimation.h"

/*
 * MaterialAnimation.h
 *
 * The DACOM "MaterialAnimation" component, implementing IMaterialAnimation. A
 * plain DACOM component (the IDAComponent / aggregation plumbing is supplied by
 * the DAComponentAggregate<> wrapper); the registration hooks are the
 * hand-written Register_/Shutdown_MaterialAnimation in MaterialAnimation.cpp.
 */
#define CLSID_MaterialAnimation "MaterialAnimation"
struct MaterialAnimation : public IMaterialAnimation
{
	BEGIN_DACOM_MAP_INBOUND(MaterialAnimation)
		DACOM_INTERFACE_ENTRY2(IID_IMaterialAnimation, IMaterialAnimation)
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
	SHADING_DEC void Register_MaterialAnimation();
	SHADING_DEC void Shutdown_MaterialAnimation();
}

#endif // MATERIALANIMATION_H
