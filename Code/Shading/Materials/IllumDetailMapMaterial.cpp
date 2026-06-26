#include <DACOM.h>
#include "Material.h"
#include "IllumDetailMapMaterial.h"

/*
 * IllumDetailMapMaterial.cpp
 *
 * shading.dll - the DACOM "IllumDetailMapMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT IllumDetailMapMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( IllumDetailMapMaterial, IS_SIMPLE );
