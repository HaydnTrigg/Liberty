#pragma once

#ifndef MASKED2DETAILMAPMATERIAL_H
#define MASKED2DETAILMAPMATERIAL_H

#include <IMaterial.h>

/*
 * Masked2DetailMapMaterial.h
 *
 * The DACOM "Masked2DetailMapMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( Masked2DetailMapMaterial, IS_SIMPLE ) in
 * Masked2DetailMapMaterial.cpp.
 */
#define CLSID_Masked2DetailMapMaterial "Masked2DetailMapMaterial"
struct Masked2DetailMapMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(Masked2DetailMapMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // MASKED2DETAILMAPMATERIAL_H
