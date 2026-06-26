#pragma once

#ifndef SINGLEPASSMATERIAL_H
#define SINGLEPASSMATERIAL_H

#include <IMaterial.h>

/*
 * SinglePassMaterial.h
 *
 * The DACOM "SinglePassMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( SinglePassMaterial, IS_SIMPLE ) in
 * SinglePassMaterial.cpp.
 */
#define CLSID_SinglePassMaterial "SinglePassMaterial"
struct SinglePassMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(SinglePassMaterial)
	DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
	END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // SINGLEPASSMATERIAL_H
