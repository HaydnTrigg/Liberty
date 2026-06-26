#pragma once

#ifndef ECETMATERIAL_H
#define ECETMATERIAL_H

#include <IMaterial.h>

/*
 * EcEtMaterial.h
 *
 * The DACOM "EcEtMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( EcEtMaterial, IS_SIMPLE ) in
 * EcEtMaterial.cpp.
 */
#define CLSID_EcEtMaterial "EcEtMaterial"
struct EcEtMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(EcEtMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // ECETMATERIAL_H
