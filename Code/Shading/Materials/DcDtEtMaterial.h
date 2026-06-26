#pragma once

#ifndef DCDTETMATERIAL_H
#define DCDTETMATERIAL_H

#include <IMaterial.h>

/*
 * DcDtEtMaterial.h
 *
 * The DACOM "DcDtEtMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( DcDtEtMaterial, IS_SIMPLE ) in
 * DcDtEtMaterial.cpp.
 */
#define CLSID_DcDtEtMaterial "DcDtEt"
struct DcDtEtMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(DcDtEtMaterial)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // DCDTETMATERIAL_H
