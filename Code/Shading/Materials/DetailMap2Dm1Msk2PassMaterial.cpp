#include <DACOM.h>
#include "Material.h"
#include "DetailMap2Dm1Msk2PassMaterial.h"

/*
 * DetailMap2Dm1Msk2PassMaterial.cpp
 *
 * shading.dll - the DACOM "DetailMap2Dm1Msk2PassMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT DetailMap2Dm1Msk2PassMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( DetailMap2Dm1Msk2PassMaterial, IS_SIMPLE );
