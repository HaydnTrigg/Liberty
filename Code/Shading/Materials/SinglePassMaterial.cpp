#include <DACOM.h>
#include "Material.h"
#include "SinglePassMaterial.h"

/*
 * SinglePassMaterial.cpp
 *
 * shading.dll - the DACOM "SinglePassMaterial" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT SinglePassMaterial::init(DACOMDESC* info)
{
	return GR_OK;
}

DECLARE_MATERIAL( SinglePassMaterial, IS_AGGREGATE);

#define CLSID_DcDtMaterial "DcDt"
DECLARE_MATERIAL_WITH_IMPL( DcDtMaterial, SinglePassMaterial, IS_AGGREGATE);

#define CLSID_DcDtEcMaterial "DcDtEc"
DECLARE_MATERIAL_WITH_IMPL( DcDtEcMaterial, SinglePassMaterial, IS_AGGREGATE);

#define CLSID_DcDtOcOtMaterial "DcDtOcOt"
DECLARE_MATERIAL_WITH_IMPL( DcDtOcOtMaterial, SinglePassMaterial, IS_AGGREGATE);

#define CLSID_DcDtEcOcOtMaterial "DcDtEcOcOt"
DECLARE_MATERIAL_WITH_IMPL( DcDtEcOcOtMaterial, SinglePassMaterial, IS_AGGREGATE);
