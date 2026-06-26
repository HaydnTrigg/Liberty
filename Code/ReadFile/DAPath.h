#pragma once

// A component that implements a data file search path.

#ifndef __DAPATH_H__
#define __DAPATH_H__
#ifdef SEARCH_PATH_FILESYSTEM

#include "DACOM.h"
#include "FileSys.h"
#include "SearchPath.h"
#include "TSmartPointer.h"

struct DACOM_NO_VTABLE DAPath : public ISearchPath
{
	BEGIN_DACOM_MAP_INBOUND(DAPath)
		DACOM_INTERFACE_ENTRY(ISearchPath)
		DACOM_INTERFACE_ENTRY(IComponentFactory)
		DACOM_INTERFACE_ENTRY2(IID_ISearchPath, ISearchPath)
		DACOM_INTERFACE_ENTRY2(IID_IComponentFactory, IComponentFactory)
		END_DACOM_MAP()

	//--------------------------
	// data items
	//--------------------------

	char path[MAX_PATH + 4];
	COMPTR<IFileSystem>* systems;
	int numSystems;

	~DAPath(void)
	{
		reset();
	}

	DA_HEAP_DEFINE_NEW_OPERATOR(DAPath);

	GENRESULT init(SEARCHPATHDESC* desc)
	{
		return GR_OK;
	}

	void reset(void)
	{
		delete[] systems;
		numSystems = 0;
		path[0] = 0;
	}

	// *** IComponentFactory methods ***

	DEFMETHOD(CreateInstance) (DACOMDESC* descriptor, void** instance);

	// *** ISearchPath methods ***

	DEFMETHOD(SetPath) (const C8* path);
	DEFMETHOD_(U32, GetPath) (C8* path, U32 bufferSize) const;

	// *** DAPath methods ***

	static bool DAPath::createFileSystem(IComponentFactory* factory, char* buffer, IFileSystem** resultFile);
	static int __stdcall parse(char* buffer, COMPTR<IFileSystem>*& ptrs, int recurseCount = 0);
};

#endif // SEARCH_PATH_FILESYSTEM
#endif // __DAPATH_H__
