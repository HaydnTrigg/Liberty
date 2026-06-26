#include <DACOM.h>
#include "Material.h"
#include "DetailMapMaterial.h"

/*
 * DetailMapMaterial.cpp
 *
 * shading.dll - the DACOM "DetailMapMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT DetailMapMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( DetailMapMaterial, IS_SIMPLE );
