#include <DACOM.h>
#include "Material.h"
#include "SinglePassTwoMaterial.h"

/*
 * SinglePassTwoMaterial.cpp
 *
 * shading.dll - the DACOM "SinglePassTwoMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT SinglePassTwoMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( SinglePassTwoMaterial, IS_AGGREGATE);

#define CLSID_DcDtTwoMaterial "DcDtTwo"
DECLARE_MATERIAL_WITH_IMPL( DcDtTwoMaterial, SinglePassTwoMaterial, IS_AGGREGATE);

#define CLSID_DcDtEcTwoMaterial "DcDtEcTwo"
DECLARE_MATERIAL_WITH_IMPL( DcDtEcTwoMaterial, SinglePassTwoMaterial, IS_AGGREGATE);

#define CLSID_DcDtOcOtTwoMaterial "DcDtOcOtTwo"
DECLARE_MATERIAL_WITH_IMPL( DcDtOcOtTwoMaterial, SinglePassTwoMaterial, IS_AGGREGATE);

#define CLSID_DcDtEcOcOtTwoMaterial "DcDtEcOcOtTwo"
DECLARE_MATERIAL_WITH_IMPL( DcDtEcOcOtTwoMaterial, SinglePassTwoMaterial, IS_AGGREGATE);
