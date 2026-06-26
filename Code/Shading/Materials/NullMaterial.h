#pragma once

#ifndef NULLMATERIAL_H
#define NULLMATERIAL_H

#include <IMaterial.h>

/*
 * NullMaterial.h
 *
 * The DACOM "NullMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( NullMaterial, IS_SIMPLE ) in
 * NullMaterial.cpp.
 */
#define CLSID_NullMaterial "NullMaterial"
struct NullMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(NullMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // NULLMATERIAL_H
