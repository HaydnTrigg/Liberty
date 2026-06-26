#pragma once

#ifndef IMATERIAL_H
#define IMATERIAL_H

#include <DACOM.h>

/*
 * IMaterial.h
 *
 * Public DACOM interface implemented by shading.dll's Material component.
 * Currently empty - a placeholder modelled on System's interface headers while
 * the shading.dll reimplementation is built up.
 */

#define IID_IMaterial DACOM_MAKE_IID("IMaterial")
DACOM_INTERFACE(IMaterial, IID_IMaterial);

struct DACOM_NO_VTABLE IMaterial : public IDAComponent
{
};

#endif // IMATERIAL_H
