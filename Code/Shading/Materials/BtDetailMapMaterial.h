#pragma once

#ifndef BTDETAILMAPMATERIAL_H
#define BTDETAILMAPMATERIAL_H

#include <IMaterial.h>

/*
 * BtDetailMapMaterial.h
 *
 * The DACOM "BtDetailMapMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( BtDetailMapMaterial, IS_SIMPLE ) in
 * BtDetailMapMaterial.cpp.
 */
#define CLSID_BtDetailMapMaterial "BtDetailMapMaterial"
struct BtDetailMapMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(BtDetailMapMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // BTDETAILMAPMATERIAL_H
