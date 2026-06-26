#include <DACOM.h>
#include <IMaterial.h>

#include "Material.h"

/*
 * Material.cpp
 *
 * shading.dll - the DACOM "Material" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT Material::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( Material, IS_SIMPLE );
