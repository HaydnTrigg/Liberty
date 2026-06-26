#include <DACOM.h>
#include "Material.h"
#include "NebulaTwoMaterial.h"

/*
 * NebulaTwoMaterial.cpp
 *
 * shading.dll - the DACOM "NebulaTwoMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT NebulaTwoMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( NebulaTwoMaterial, IS_SIMPLE );
