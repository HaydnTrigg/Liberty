#include <DACOM.h>
#include "Material.h"
#include "LightMapMaterial.h"

/*
 * LightMapMaterial.cpp
 *
 * shading.dll - the DACOM "LightMapMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT LightMapMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( LightMapMaterial, IS_SIMPLE );
