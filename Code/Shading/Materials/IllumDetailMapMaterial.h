#pragma once

#ifndef ILLUMDETAILMAPMATERIAL_H
#define ILLUMDETAILMAPMATERIAL_H

#include <IMaterial.h>

/*
 * IllumDetailMapMaterial.h
 *
 * The DACOM "IllumDetailMapMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( IllumDetailMapMaterial, IS_SIMPLE ) in
 * IllumDetailMapMaterial.cpp.
 */
#define CLSID_IllumDetailMapMaterial "IllumDetailMapMaterial"
struct IllumDetailMapMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(IllumDetailMapMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // ILLUMDETAILMAPMATERIAL_H
