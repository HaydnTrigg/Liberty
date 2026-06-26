#include <DACOM.h>

#include "MaterialBatcher.h"

/*
 * MaterialBatcher.cpp
 *
 * shading.dll - the DACOM "MaterialBatcher" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT MaterialBatcher::init(AGGDESC* info)
{
	return GR_OK;
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//
//
// DllMain (Shading/DllMain.cpp) calls these registration hooks.

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_materialBatcherFactory = nullptr;

extern "C"
{
	/*
	 * Registers the MaterialBatcher component factory with the component manager.
	 */
	void Register_MaterialBatcher()
	{
		g_materialBatcherFactory = RegisterComponentFactory<DAComponentAggregate<MaterialBatcher>>(DACOM_LIBRARY_NAME, CLSID_MaterialBatcher, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the MaterialBatcher component factory.
	 */
	void Shutdown_MaterialBatcher()
	{
		if (g_materialBatcherFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_materialBatcherFactory, CLSID_MaterialBatcher);
			g_materialBatcherFactory = nullptr;
		}
	}
}
