#include <DACOM.h>
#include "Material.h"
#include "AtmosphereMaterial.h"

/*
 * AtmosphereMaterial.cpp
 *
 * shading.dll - the DACOM "AtmosphereMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT AtmosphereMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( AtmosphereMaterial, IS_SIMPLE );
