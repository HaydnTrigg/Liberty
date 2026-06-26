#pragma once

#ifndef ATMOSPHEREMATERIAL_H
#define ATMOSPHEREMATERIAL_H

#include <IMaterial.h>

/*
 * AtmosphereMaterial.h
 *
 * The DACOM "AtmosphereMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( AtmosphereMaterial, IS_SIMPLE ) in
 * AtmosphereMaterial.cpp.
 */
#define CLSID_AtmosphereMaterial "AtmosphereMaterial"
struct AtmosphereMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(AtmosphereMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // ATMOSPHEREMATERIAL_H
