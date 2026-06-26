#include <DACOM.h>
#include "Material.h"
#include "NullMaterial.h"

/*
 * NullMaterial.cpp
 *
 * shading.dll - the DACOM "NullMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT NullMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( NullMaterial, IS_SIMPLE );
