#include <DACOM.h>
#include "Material.h"
#include "EcEtMaterial.h"

/*
 * EcEtMaterial.cpp
 *
 * shading.dll - the DACOM "EcEtMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT EcEtMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( EcEtMaterial, IS_SIMPLE );
