#include <DACOM.h>

#include "MaterialLibrary.h"

/*
 * MaterialLibrary.cpp
 *
 * shading.dll - the DACOM "MaterialLibrary" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT MaterialLibrary::init(AGGDESC* info)
{
	return GR_OK;
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//
//
// DllMain (Shading/DllMain.cpp) calls these registration hooks.

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_materialLibraryFactory = nullptr;

extern "C"
{
	/*
	 * Registers the MaterialLibrary component factory with the component manager.
	 */
	void Register_MaterialLibrary()
	{
		g_materialLibraryFactory = RegisterComponentFactory<DAComponentAggregate<MaterialLibrary>>(DACOM_LIBRARY_NAME, CLSID_MaterialLibrary, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the MaterialLibrary component factory.
	 */
	void Shutdown_MaterialLibrary()
	{
		if (g_materialLibraryFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_materialLibraryFactory, CLSID_MaterialLibrary);
			g_materialLibraryFactory = nullptr;
		}
	}
}
