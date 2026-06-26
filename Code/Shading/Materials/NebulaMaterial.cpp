#include <DACOM.h>
#include "Material.h"
#include "NebulaMaterial.h"

/*
 * NebulaMaterial.cpp
 *
 * shading.dll - the DACOM "NebulaMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT NebulaMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( NebulaMaterial, IS_SIMPLE );
