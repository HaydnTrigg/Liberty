#pragma once

#ifndef LIGHTMAPMATERIAL_H
#define LIGHTMAPMATERIAL_H

#include <IMaterial.h>

/*
 * LightMapMaterial.h
 *
 * The DACOM "LightMapMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( LightMapMaterial, IS_SIMPLE ) in
 * LightMapMaterial.cpp.
 */
#define CLSID_LightMapMaterial "LightMapMaterial"
struct LightMapMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(LightMapMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // LIGHTMAPMATERIAL_H
