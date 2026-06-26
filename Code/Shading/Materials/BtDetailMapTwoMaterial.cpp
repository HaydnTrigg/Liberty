#include <DACOM.h>
#include "Material.h"
#include "BtDetailMapTwoMaterial.h"

/*
 * BtDetailMapTwoMaterial.cpp
 *
 * shading.dll - the DACOM "BtDetailMapTwoMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT BtDetailMapTwoMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( BtDetailMapTwoMaterial, IS_SIMPLE );
