#pragma once

// A component that implements a data file search path.

#ifndef __SEARCHPATH_H__
#define __SEARCHPATH_H__

#include "DACOM.h"

#define SEARCH_PATH_INTERFACE_NAME "SearchPath"
#define IID_ISearchPath DACOM_MAKE_IID("ISearchPath")

struct SEARCHPATHDESC : DACOMDESC
{
	SEARCHPATHDESC(const C8* _interface_name = SEARCH_PATH_INTERFACE_NAME) :
		DACOMDESC(_interface_name)
	{
	}
};

struct DACOM_NO_VTABLE ISearchPath : public IComponentFactory
{
	// *** IDAComponent methods ***

	DEFMETHOD(QueryInterface) (const C8* interface_name, void** instance) = 0;
	DEFMETHOD_(U32, AddRef)           (void) = 0;
	DEFMETHOD_(U32, Release)          (void) = 0;

	// *** IComponentFactory methods ***

	DEFMETHOD(CreateInstance) (DACOMDESC* descriptor, void** instance) = 0;

	// *** ISearchPath methods ***

	DEFMETHOD(SetPath) (const C8* path) = 0;
	DEFMETHOD_(U32, GetPath) (C8* path, U32 bufferSize) const = 0;
};

extern "C"
{
	READFILE_DEC IComponentFactory* CreateSearchPathFactory(void);
	READFILE_DEC void Register_SearchPath();
}

#endif // __SEARCHPATH_H__
