#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include "IMaterial.h"

/*
 * Material.h
 *
 * The DACOM "Material" component, implementing IMaterial. A plain DACOM component
 * (the IDAComponent plumbing is supplied by the DAComponent<> wrapper); the
 * registration hooks are emitted by DECLARE_MATERIAL( Material, IS_SIMPLE ) in
 * Material.cpp.
 */
#define CLSID_Material "Material"
struct Material : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(Material)
		DACOM_INTERFACE_ENTRY2(IID_IMaterial, IMaterial)
		END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(DACOMDESC* info);
};

//
// Type macros used by DECLARE_MATERIAL
//
#define IS_AGGREGATE(cls) DAComponentFactory2< DAComponentAggregate<cls>, AGGDESC >
#define IS_SIMPLE(cls) DAComponentFactory< DAComponent<cls>, DACOMDESC >


// DECLARE_MATERIAL
//
// Use this macro to declare the code to register this material.
// See the standard materials for examples of how to use this macro.
//
// The Register_/Shutdown_ pair behaves like the hand-written registration
// hooks it replaces: Register_ creates the component factory and registers it
// with DACOM, retaining the factory pointer so Shutdown_ can later unregister
// it.
//
#define DECLARE_MATERIAL(comp,type) \
static IComponentFactory *s ## comp = nullptr;	\
bool Register_ ## comp( void ) \
{\
	s ## comp = new type(comp)( CLSID_ ## comp );\
	if( s ## comp == NULL ) {\
		return false;\
	}\
	DACOM_Acquire()->RegisterComponent( s ## comp, CLSID_ ## comp, DACOM_NORMAL_PRIORITY );	\
	s ## comp ->Release();	\
	return true; \
}\
void Shutdown_ ## comp( void ) \
{\
	if( s ## comp != NULL ) {\
		DACOM_Acquire()->UnregisterComponent( s ## comp, CLSID_ ## comp );\
		s ## comp = nullptr;\
	}\
}

// DECLARE_MATERIAL_WITH_IMPL
//
// Use this macro to declare the code to register this material.
// See the standard materials for examples of how to use this macro.
//
#define DECLARE_MATERIAL_WITH_IMPL(mtl,impl,type) \
static IComponentFactory *s ## mtl = nullptr;	\
bool Register_ ## mtl( void ) \
{\
	s ## mtl = new type(impl)( CLSID_ ## mtl );\
	if( s ## mtl == NULL ) {\
		return false;\
	}\
	DACOM_Acquire()->RegisterComponent( s ## mtl, CLSID_ ## mtl, DACOM_NORMAL_PRIORITY );	\
	s ## mtl ->Release();	\
	return true; \
}\
void Shutdown_ ## mtl( void ) \
{\
	if( s ## mtl != NULL ) {\
		DACOM_Acquire()->UnregisterComponent( s ## mtl, CLSID_ ## mtl );\
		s ## mtl = nullptr;\
	}\
}

// REGISTER_MATERIAL
//
// Place a call to this macro in the DllMain in Materials.cpp
// to register a material component.  See DllMain() for more information.
//
#define REGISTER_MATERIAL(comp) \
extern bool Register_ ## comp( void );	\
	Register_ ## comp();

// SHUTDOWN_MATERIAL
//
// Place a call to this macro in the DllMain in Materials.cpp to unregister a
// material component previously registered with REGISTER_MATERIAL.
//
#define SHUTDOWN_MATERIAL(comp) \
extern void Shutdown_ ## comp( void );	\
	Shutdown_ ## comp();

#endif // MATERIAL_H
