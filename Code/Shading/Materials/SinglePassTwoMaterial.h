#pragma once

#ifndef SINGLEPASSTWOMATERIAL_H
#define SINGLEPASSTWOMATERIAL_H

#include <IMaterial.h>
#include "SinglePassMaterial.h"

/*
 * SinglePassTwoMaterial.h
 *
 * The DACOM "SinglePassTwoMaterial" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( SinglePassTwoMaterial, IS_SIMPLE ) in
 * SinglePassTwoMaterial.cpp.
 */
#define CLSID_SinglePassTwoMaterial "SinglePassTwoMaterial"
struct SinglePassTwoMaterial : public SinglePassMaterial
{
	BEGIN_DACOM_MAP_INBOUND(SinglePassTwoMaterial)
	DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
	END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

#endif // SINGLEPASSTWOMATERIAL_H
