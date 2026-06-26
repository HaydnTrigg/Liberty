#pragma once

#ifndef IMATERIALBATCHER_H
#define IMATERIALBATCHER_H

#include <DACOM.h>

/*
 * IMaterialBatcher.h
 *
 * Public DACOM interface implemented by shading.dll's MaterialBatcher component.
 * Currently empty - a placeholder modelled on System's interface headers while
 * the shading.dll reimplementation is built up.
 */

#define IID_IMaterialBatcher DACOM_MAKE_IID("IMaterialBatcher")
DACOM_INTERFACE(IMaterialBatcher, IID_IMaterialBatcher);

struct DACOM_NO_VTABLE IMaterialBatcher : public IDAComponent
{
};

#endif // IMATERIALBATCHER_H
