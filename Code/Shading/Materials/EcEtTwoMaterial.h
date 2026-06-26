#pragma once

#ifndef ECETTWOMATERIAL_H
#define ECETTWOMATERIAL_H

#include <IMaterial.h>

/*
 * EcEtTwoMaterial.h
 *
 * The DACOM "EcEtTwoMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( EcEtTwoMaterial, IS_SIMPLE ) in
 * EcEtTwoMaterial.cpp.
 */
#define CLSID_EcEtTwoMaterial "EcEtTwoMaterial"
struct EcEtTwoMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(EcEtTwoMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // ECETTWOMATERIAL_H
