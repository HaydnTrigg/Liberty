#include <DACOM.h>

#include "MaterialAnimation.h"

/*
 * MaterialAnimation.cpp
 *
 * shading.dll - the DACOM "MaterialAnimation" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT MaterialAnimation::init(AGGDESC* info)
{
	return GR_OK;
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//
//
// DllMain (Shading/DllMain.cpp) calls these registration hooks.

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_materialAnimationFactory = nullptr;

extern "C"
{
	/*
	 * Registers the MaterialAnimation component factory with the component manager.
	 */
	void Register_MaterialAnimation()
	{
		g_materialAnimationFactory = RegisterComponentFactory<DAComponentAggregate<MaterialAnimation>>(DACOM_LIBRARY_NAME, CLSID_MaterialAnimation, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the MaterialAnimation component factory.
	 */
	void Shutdown_MaterialAnimation()
	{
		if (g_materialAnimationFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_materialAnimationFactory, CLSID_MaterialAnimation);
			g_materialAnimationFactory = nullptr;
		}
	}
}
