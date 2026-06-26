#pragma once

#ifndef NEBULATWOMATERIAL_H
#define NEBULATWOMATERIAL_H

#include <IMaterial.h>

/*
 * NebulaTwoMaterial.h
 *
 * The DACOM "NebulaTwoMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( NebulaTwoMaterial, IS_SIMPLE ) in
 * NebulaTwoMaterial.cpp.
 */
#define CLSID_NebulaTwoMaterial "NebulaTwoMaterial"
struct NebulaTwoMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(NebulaTwoMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // NEBULATWOMATERIAL_H
