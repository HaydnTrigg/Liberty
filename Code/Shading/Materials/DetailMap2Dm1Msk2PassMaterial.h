#pragma once

#ifndef DETAILMAP2DM1MSK2PASSMATERIAL_H
#define DETAILMAP2DM1MSK2PASSMATERIAL_H

#include <IMaterial.h>

/*
 * DetailMap2Dm1Msk2PassMaterial.h
 *
 * The DACOM "DetailMap2Dm1Msk2PassMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( DetailMap2Dm1Msk2PassMaterial, IS_SIMPLE ) in
 * DetailMap2Dm1Msk2PassMaterial.cpp.
 */
#define CLSID_DetailMap2Dm1Msk2PassMaterial "DetailMap2Dm1Msk2PassMaterial"
struct DetailMap2Dm1Msk2PassMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(DetailMap2Dm1Msk2PassMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // DETAILMAP2DM1MSK2PASSMATERIAL_H
