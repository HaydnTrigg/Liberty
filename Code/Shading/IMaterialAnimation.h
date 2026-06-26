#pragma once

#ifndef IMATERIALANIMATION_H
#define IMATERIALANIMATION_H

#include <DACOM.h>

/*
 * IMaterialAnimation.h
 *
 * Public DACOM interface implemented by shading.dll's MaterialAnimation component.
 * Currently empty - a placeholder modelled on System's interface headers while
 * the shading.dll reimplementation is built up.
 */

#define IID_IMaterialAnimation DACOM_MAKE_IID("IMaterialAnimation")
DACOM_INTERFACE(IMaterialAnimation, IID_IMaterialAnimation);

struct DACOM_NO_VTABLE IMaterialAnimation : public IDAComponent
{
};

#endif // IMATERIALANIMATION_H
