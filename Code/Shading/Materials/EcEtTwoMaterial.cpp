#include <DACOM.h>
#include "Material.h"
#include "EcEtTwoMaterial.h"

/*
 * EcEtTwoMaterial.cpp
 *
 * shading.dll - the DACOM "EcEtTwoMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT EcEtTwoMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( EcEtTwoMaterial, IS_SIMPLE );
