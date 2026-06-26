#pragma once

#ifndef DETAILMAPMATERIAL_H
#define DETAILMAPMATERIAL_H

#include <IMaterial.h>

/*
 * DetailMapMaterial.h
 *
 * The DACOM "DetailMapMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( DetailMapMaterial, IS_SIMPLE ) in
 * DetailMapMaterial.cpp.
 */
#define CLSID_DetailMapMaterial "DetailMapMaterial"
struct DetailMapMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(DetailMapMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // DETAILMAPMATERIAL_H
