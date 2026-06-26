#pragma once

#ifndef NEBULAMATERIAL_H
#define NEBULAMATERIAL_H

#include <IMaterial.h>

/*
 * NebulaMaterial.h
 *
 * The DACOM "NebulaMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( NebulaMaterial, IS_SIMPLE ) in
 * NebulaMaterial.cpp.
 */
#define CLSID_NebulaMaterial "NebulaMaterial"
struct NebulaMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(NebulaMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // NEBULAMATERIAL_H
