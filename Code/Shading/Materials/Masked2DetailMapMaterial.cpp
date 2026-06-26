#include <DACOM.h>
#include "Material.h"
#include "Masked2DetailMapMaterial.h"

/*
 * Masked2DetailMapMaterial.cpp
 *
 * shading.dll - the DACOM "Masked2DetailMapMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT Masked2DetailMapMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( Masked2DetailMapMaterial, IS_SIMPLE );
