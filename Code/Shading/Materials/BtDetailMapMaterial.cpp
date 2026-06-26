#include <DACOM.h>
#include "Material.h"
#include "BtDetailMapMaterial.h"

/*
 * BtDetailMapMaterial.cpp
 *
 * shading.dll - the DACOM "BtDetailMapMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT BtDetailMapMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( BtDetailMapMaterial, IS_SIMPLE );
