#pragma once

#ifndef BTDETAILMAPTWOMATERIAL_H
#define BTDETAILMAPTWOMATERIAL_H

#include <IMaterial.h>

/*
 * BtDetailMapTwoMaterial.h
 *
 * The DACOM "BtDetailMapTwoMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( BtDetailMapTwoMaterial, IS_SIMPLE ) in
 * BtDetailMapTwoMaterial.cpp.
 */
#define CLSID_BtDetailMapTwoMaterial "BtDetailMapTwoMaterial"
struct BtDetailMapTwoMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(BtDetailMapTwoMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // BTDETAILMAPTWOMATERIAL_H
