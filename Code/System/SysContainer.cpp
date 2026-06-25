#include <DACOM.h>
#include <TSmartPointer.h>

#include "ISystem.h"
#include "IConnection.h"

#include <string.h>

/*
 * SysContainer.cpp
 *
 * system.dll - the DACOM "SystemContainer" component.
 *
 * The SystemContainer is created by the engine from an AGGDESC whose
 * interface_name is "SystemContainer". On LoadSystemComponents() it walks the
 * "[System]" section of the profile, CreateInstance()s each named component and
 * hangs it off an internal list; thereafter it fans Initialize() / Update() out
 * to those children and aggregates their IDAConnectionPointContainer interfaces.
 *
 * This is a clean reimplementation ported from Conquest's SysContainer.cpp (the
 * /Libs/dev/Src/System variant) and reconciled method-by-method against the
 * shipped Freelancer system.dll (system.dll.i64). Notable differences from the
 * Conquest source, dictated by the shipped binary:
 *   - ISystemComponent has no ShutdownAggregate(); Shutdown() just releases and
 *     deletes each list element.
 *   - The inbound DACOM map keys on the versioned IID strings ("1.11_*").
 *   - No Conquest "CQPipeline" element is appended in LoadSystemComponents().
 *   - The component registers under the name "SystemContainer".
 */

// Handle to the component manager, acquired in Register_SystemContainer() and
// used by LoadSystemComponents() / AddComponent() to CreateInstance children.
struct SystemContainer;

/*
 * Inner (aggregation) component. Overrides QueryInterface so that, after the
 * container's own interfaces are checked, the request is also offered to every
 * loaded child component in priority order.
 */
struct SysConInner : public DAComponentInner<SystemContainer>
{
	SysConInner(SystemContainer* _owner) : DAComponentInner<SystemContainer>(_owner)
	{
	}

	DACOM_DEFMETHOD(QueryInterface) (const C8* interface_name, void** instance);
};

/*
 * SystemContainer
 *
 * The DACOM system-container component. The base-class order, virtual method
 * order and signatures define the DACOM vtables and must not be reordered or
 * changed.
 */
#define CLSID_SystemContainer "SystemContainer"
struct SystemContainer : public ISystemContainer, IDAConnectionPointContainer
{
	/*
	 * One loaded child component. pInner holds the reference; pSysComp / pAggComp
	 * cache the optional ISystemComponent / IAggregateComponent views used to
	 * drive Update() and Initialize().
	 */
	struct ELEMENT
	{
		struct ELEMENT* pNext;
		COMPTR<IDAComponent>  pInner;
		IDAComponent* pOuter;
		ISystemComponent* pSysComp;
		IAggregateComponent* pAggComp;

		ELEMENT(void)
		{
			pNext = 0;
		}
	};

	BEGIN_DACOM_MAP_INBOUND(SystemContainer)
		DACOM_INTERFACE_ENTRY2(IID_ISystemContainer, ISystemContainer)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent, IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_ISystemComponent, ISystemComponent)
		DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
		END_DACOM_MAP()

	ELEMENT* pList, * pLast;
	SysConInner innerComponent;
	IDAComponent* outerComponent;
	BOOL32 bAggMember, bLoaded;

	SystemContainer(AGGDESC* info) : innerComponent(this)
	{
		outerComponent = &innerComponent;
	};

	~SystemContainer(void);

	DA_HEAP_DEFINE_NEW_OPERATOR(SystemContainer);

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(AGGDESC* info);

	// *** IDAComponent methods ***

	DACOM_DEFMETHOD(QueryInterface) (const C8* interface_name, void** instance)
	{
		return outerComponent->QueryInterface(interface_name, instance);
	}

	DACOM_DEFMETHOD_(U32, AddRef) (void)
	{
		return outerComponent->AddRef();
	}

	DACOM_DEFMETHOD_(U32, Release) (void)
	{
		return outerComponent->Release();
	}

	// *** ISystemContainer methods ***

	DACOM_DEFMETHOD(LoadSystemComponents) (void);
	DACOM_DEFMETHOD(Shutdown) (void);
	DACOM_DEFMETHOD(AddComponent) (const AGGDESC* descriptor);

	// *** IAggregateComponent methods ***

	DACOM_DEFMETHOD(Initialize) (void);

	// *** ISystemComponent methods ***

	DACOM_DEFMETHOD_(void, Update) (void);

	// *** IDAConnectionPointContainer methods ***

	DACOM_DEFMETHOD(FindConnectionPoint) (const C8* connectionName, struct IDAConnectionPoint** connPoint);
	DACOM_DEFMETHOD_(BOOL32, EnumerateConnectionPoints) (CONNCONTAINER_ENUM_PROC proc, void* context = 0);

	// Returns the most-derived IDAComponent base (used as the aggregation outer).
	IDAComponent* getBase(void)
	{
		return static_cast<ISystemContainer*>(this);
	}
};

/*
 * Resolves the container's own interfaces through the inbound map, then offers
 * the request to each loaded child in priority order.
 */
GENRESULT SysConInner::QueryInterface(const C8* interface_name, void** instance)
{
	GENRESULT result;

	if ((result = DAComponentInner<SystemContainer>::QueryInterface(interface_name, instance)) == GR_OK)
		return result;

	SystemContainer::ELEMENT* tmp = owner->pList;

	while (tmp)
	{
		if ((result = tmp->pInner->QueryInterface(interface_name, instance)) == GR_OK)
			return result;

		tmp = tmp->pNext;
	}

	return GR_INTERFACE_UNSUPPORTED;
}

/*
 * DACOM factory hook, called immediately after construction. Rejects an explicit
 * "SystemContainer" description (to avoid recursive self-instantiation) and
 * records the aggregation links when created as an aggregate member.
 */
GENRESULT SystemContainer::init(AGGDESC* info)
{
	if (info->description != 0 && strcmp(info->description, "SystemContainer") == 0)
		return GR_GENERIC;

	if (info->outer)
	{
		outerComponent = info->outer;
		*(info->inner) = &innerComponent;
		bAggMember = 1;
	}

	return GR_OK;
}

/*
 * Self-AddRef (so the recursive Release inside Shutdown cannot re-enter
 * destruction) then tears down the child list.
 */
SystemContainer::~SystemContainer(void)
{
	AddRef();		// prevent infinite loop
	Shutdown();
}

/*
 * Releases and deletes every loaded child component.
 */
GENRESULT SystemContainer::Shutdown(void)
{
	ELEMENT* tmp;

	while (pList)
	{
		tmp = pList->pNext;
		delete pList;
		pList = tmp;
	}

	pLast = 0;
	bLoaded = 0;

	return GR_OK;
}

/*
 * Reads the "[System]" profile section and CreateInstance()s each listed
 * component, hanging it off the child list. A child that exposes
 * ISystemComponent / IAggregateComponent is cached for Update() / Initialize().
 * Once the list is built, non-aggregate containers Initialize() it immediately.
 */
GENRESULT SystemContainer::LoadSystemComponents(void)
{
	if (bLoaded == 0)
	{
		COMPTR<IProfileParser> parser;
		HANDLE hSection;
		char buffer[256];
		int line = 0;

		ICOManager* DACOM = DACOM_Acquire();

		if (DACOM->QueryInterface(IID_IProfileParser, parser) != GR_OK)
			goto Done;

		if ((hSection = parser->CreateSection("System")) == 0)
			goto Done;

		while (parser->ReadProfileLine(hSection, line++, buffer, sizeof(buffer)) != 0)
		{
			char* ptr, * ptr2;
			AGGDESC info;

			ptr = buffer;
			while (*ptr == ' ' || *ptr == '\t')
				ptr++;
			if (*ptr == ';' || *ptr == 0)
				continue;
			if ((ptr2 = strchr(ptr, '=')) != 0)
			{
				*ptr2++ = 0;
				while (*ptr2 == ' ' || *ptr2 == '\t')
					ptr2++;
			}

			ELEMENT* element = new ELEMENT;

			info.interface_name = ptr;
			info.outer = getBase();
			info.inner = element->pInner;
			info.description = ptr2;

			// Trim the interface name only after the descriptor points at it.
			if ((ptr = const_cast<char*>(strchr(info.interface_name, ' '))) != 0)
				*ptr = 0;
			if ((ptr = const_cast<char*>(strchr(info.interface_name, '\t'))) != 0)
				*ptr = 0;

			const char* response = "";

			if (DACOM->CreateInstance(&info, (void**)&element->pOuter) != GR_OK)
			{
				response = "[FAILED]";
				delete element;
			}
			else
			{
				response = "[OK]";

				if (pLast)
					pLast->pNext = element;
				else
					pList = element;
				pLast = element;

				if (element->pInner->QueryInterface(IID_ISystemComponent, (void**)&element->pSysComp) == GR_OK)
					element->pSysComp->Release();		// drop the extra QueryInterface reference
				if (element->pInner->QueryInterface(IID_IAggregateComponent, (void**)&element->pAggComp) == GR_OK)
					element->pAggComp->Release();		// drop the extra QueryInterface reference
			}

			GENERAL_NOTICE(TEMPSTR("SystemContainer: LoadSystemComponents: Loading '%s' [%s] returned %s\n",
				info.interface_name,
				info.description ? info.description : "",
				response));
		}

		if (bAggMember == 0)	// otherwise the outer container drives Initialize()
			Initialize();
		bLoaded = 1;
	}

Done:
	return GR_OK;
}

/*
 * Creates and links one additional component from a caller-supplied descriptor.
 * The descriptor's outer/inner fields are borrowed for the CreateInstance call
 * and restored before returning.
 */
GENRESULT SystemContainer::AddComponent(const AGGDESC* descriptor)
{
	ELEMENT* element = 0;
	AGGDESC* info = (AGGDESC*)descriptor;
	GENRESULT result = GR_OK;
	IDAComponent* outer;
	IDAComponent** inner;

	if (descriptor == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	outer = info->outer;		// save the caller's values
	inner = info->inner;

	if ((element = new ELEMENT) == 0)
	{
		result = GR_OUT_OF_MEMORY;
		goto Done;
	}

	info->outer = getBase();
	info->inner = element->pInner;


	ICOManager* DACOM = DACOM_Acquire();
	if ((result = DACOM->CreateInstance(info, (void**)&element->pOuter)) != GR_OK)
	{
		info->outer = outer;		// restore the caller's values
		info->inner = inner;
		goto Done;
	}

	if (pLast)
		pLast->pNext = element;
	else
		pList = element;
	pLast = element;

	if (element->pInner->QueryInterface(IID_ISystemComponent, (void**)&element->pSysComp) == GR_OK)
		element->pSysComp->Release();		// drop the extra QueryInterface reference
	if (element->pInner->QueryInterface(IID_IAggregateComponent, (void**)&element->pAggComp) == GR_OK)
		element->pAggComp->Release();		// drop the extra QueryInterface reference

	info->outer = outer;		// restore the caller's values
	info->inner = inner;

Done:
	if (result != GR_OK)
		delete element;
	return result;
}

/*
 * Forwards a per-frame tick to every child that implements ISystemComponent.
 */
void SystemContainer::Update(void)
{
	ELEMENT* tmp;

	tmp = pList;
	while (tmp)
	{
		if (tmp->pSysComp)
			tmp->pSysComp->Update();
		tmp = tmp->pNext;
	}
}

/*
 * Initializes each aggregate child; a child that fails Initialize() is unlinked
 * and destroyed. When this container is itself an aggregate member, the first
 * failure stops the pass.
 */
GENRESULT SystemContainer::Initialize(void)
{
	ELEMENT* tmp, * back = 0;
	GENRESULT result = GR_OK;

	tmp = pList;
	while (tmp)
	{
		if (tmp->pAggComp && (result = tmp->pAggComp->Initialize()) != GR_OK)
		{
			if (back)
				back->pNext = tmp->pNext;
			else
				pList = tmp->pNext;

			delete tmp;

			if (bAggMember)
				break;

			tmp = pList;
			back = 0;
			continue;
		}

		back = tmp;
		tmp = tmp->pNext;
	}

	return result;
}

/*
 * Connection-point lookup: returns the first child connection-point container
 * that resolves 'connectionName'.
 */
GENRESULT SystemContainer::FindConnectionPoint(const C8* connectionName, struct IDAConnectionPoint** connPoint)
{
	GENRESULT result = GR_GENERIC;
	COMPTR<IDAConnectionPointContainer> container;
	ELEMENT* tmp = pList;

	while (tmp)
	{
		if (tmp->pInner->QueryInterface(IID_IDAConnectionPointContainer, container) == GR_OK)
		{
			if ((result = container->FindConnectionPoint(connectionName, connPoint)) == GR_OK)
				break;
		}

		tmp = tmp->pNext;
	}

	return result;
}

/*
 * Enumerates the connection points of every child container, stopping early if
 * the callback returns FALSE.
 */
BOOL32 SystemContainer::EnumerateConnectionPoints(CONNCONTAINER_ENUM_PROC proc, void* context)
{
	BOOL32 result = 1;
	COMPTR<IDAConnectionPointContainer> container;
	ELEMENT* tmp = pList;

	while (tmp)
	{
		if (tmp->pInner->QueryInterface(IID_IDAConnectionPointContainer, container) == GR_OK)
		{
			if ((result = container->EnumerateConnectionPoints(proc, context)) == 0)
				break;
		}

		tmp = tmp->pNext;
	}

	return result;
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//
//
// DllMain (System/DllMain.cpp) calls these registration hooks.
//
// Unlike the other Liberty components, SystemContainer registers a plain
// DAComponentFactory rather than going through the RegisterComponentFactory<>
// helper: the helper builds a DAComponentFactory2 (which constructs the class
// from the descriptor), but SystemContainer is default-constructed and then
// init()ed, exactly as the shipped binary's factory does.

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_systemContainerFactory = nullptr;

extern "C"
{
	/*
	 * Acquires the component manager and registers the SystemContainer factory.
	 */
	void Register_SystemContainer()
	{
		g_systemContainerFactory = RegisterComponentFactory<SystemContainer>(DACOM_LIBRARY_NAME, CLSID_SystemContainer, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the SystemContainer factory.
	 */
	void Shutdown_SystemContainer()
	{
		if (g_systemContainerFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_systemContainerFactory, CLSID_SystemContainer);
			g_systemContainerFactory = nullptr;
		}
	}
}
