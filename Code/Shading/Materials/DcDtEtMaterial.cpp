#include <DACOM.h>
#include "Material.h"
#include "DcDtEtMaterial.h"

/*
 * DcDtEt.cpp
 *
 * shading.dll - the DACOM "DcDtEt" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT DcDtEtMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( DcDtEtMaterial, IS_SIMPLE );
