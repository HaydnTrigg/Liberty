#pragma once

#ifndef IMATERIALLIBRARY_H
#define IMATERIALLIBRARY_H

#include <DACOM.h>

/*
 * IMaterialLibrary.h
 *
 * Public DACOM interface implemented by shading.dll's MaterialLibrary component.
 * Currently empty - a placeholder modelled on System's interface headers while
 * the shading.dll reimplementation is built up.
 */

#define IID_IMaterialLibrary DACOM_MAKE_IID("IMaterialLibrary")
DACOM_INTERFACE(IMaterialLibrary, IID_IMaterialLibrary);

struct DACOM_NO_VTABLE IMaterialLibrary : public IDAComponent
{
};

#endif // IMATERIALLIBRARY_H
