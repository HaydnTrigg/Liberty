#pragma once

//--------------------------------------------------------------------------//
//----------------------------IDAComponent Interface------------------------//
//--------------------------------------------------------------------------//

// Self-initializing structure used by clients to request a desired DA COM implementation
struct DACOMDESC
{
	U32 size;				// size of the structure + any extension
	const C8* interface_name;	// ASCIIZ name of requested interface

	DACOMDESC(const C8* _interface_name = 0)
	{
		size = sizeof(*this);
		interface_name = _interface_name;
	}
};

// Self-initializing structure used by clients to request a desired DA COM
// implementation. AGGDESC is used by components that support aggregation.
// A successfull call to CreateInstance() using an AGGDESC will return a pointer
// to an IAggregateComponent interface. Use QueryInterface() to retrieve more
// specific interface pointers. 
struct AGGDESC : public DACOMDESC
{
	struct IDAComponent* outer;
	struct IDAComponent** inner;
	const  C8* description;

	AGGDESC(const C8* _interfaceName = 0,
		const C8* _description = 0) : DACOMDESC(_interfaceName)
	{
		outer = 0;
		inner = 0;
		description = _description;
		size = sizeof(*this);
	}
};

#if defined(__cplusplus)
template<typename T> constexpr const char* __dacom_uuidof();
#define DACOM_PPV_ARG(ppType) (void**)ppType
#define DACOM_IID_PPV_ARGS(ppType) __dacom_uuidof<decltype(**(ppType))>, DACOM_PPV_ARG(ppType)
#define DACOM_INTERFACE(Type, IID) \
	struct Type; \
	template<> constexpr const char* __dacom_uuidof<Type>() { return IID; } \
	template<> constexpr const char* __dacom_uuidof<Type&>() { return IID; }
#endif

#define IID_IDAComponent DACOM_MAKE_IID("IDAComponent")
DACOM_INTERFACE(IDAComponent, IID_IDAComponent);
// Abstract base class from which all DA component classes must be derived
struct DACOM_NO_VTABLE IDAComponent
{
	DEFMETHOD(QueryInterface) (const C8* interface_name, void** instance) = 0;
	DEFMETHOD_(U32, AddRef) (void) = 0;
	DEFMETHOD_(U32, Release) (void) = 0;
};

#define IID_IComponentFactory DACOM_MAKE_IID("IComponentFactory")
DACOM_INTERFACE(IComponentFactory, IID_IComponentFactory);
// Abstract class from which all class factories inherit
struct DACOM_NO_VTABLE IComponentFactory : public IDAComponent
{
	DEFMETHOD(CreateInstance) (DACOMDESC* descriptor, void** instance) = 0;
};

#define IID_IAggregateComponent DACOM_MAKE_IID("IAggregateComponent")
DACOM_INTERFACE(IAggregateComponent, IID_IAggregateComponent);
// Abstract class from which all aggregatable classes inherit
struct DACOM_NO_VTABLE IAggregateComponent : public IDAComponent
{
	DEFMETHOD(Initialize) (void) = 0;
};
