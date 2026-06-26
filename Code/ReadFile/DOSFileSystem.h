#pragma once

#include "FileSys.h"

#ifndef __DOSFILESYSTEM_H__
#define __DOSFILESYSTEM_H__

#define MAX_OVERLAPPED_OPS		32
#define MAX_LOCK_WAIT			60000		// milliseconds to wait for a lock
#define CHECKDESCSIZE(x)    (x->size==sizeof(DAFILEDESC)||x->size==sizeof(DAFILEDESC)-sizeof(U32))

struct SERIAL_STRUCT
{
	LPFILESYSTEM lpSystem;
	DAFILE_SERIAL_PROC lpProc;
};

struct SEEK_STRUCT
{
	HANDLE hFileHandle;
	LONG lDistanceToMove;
	PLONG lpDistanceToMoveHigh;
	DWORD dwMoveMethod;
};

struct CREATE_MAPPING
{
	HANDLE hFileHandle;
	LPSECURITY_ATTRIBUTES lpFileMappingAttributes;
	DWORD flProtect;
	DWORD dwMaximumSizeHigh;
	DWORD dwMaximumSizeLow;
	LPCTSTR lpName;
};

struct FILE_OFFSET
{
	DWORD dwLow;
	DWORD dwHigh;

	FILE_OFFSET& operator += (FILE_OFFSET& offset)
	{
		*((__int64*)this) += *((__int64*)&offset);
		return *this;
	}

	FILE_OFFSET& operator -= (FILE_OFFSET& offset)
	{
		*((__int64*)this) -= *((__int64*)&offset);
		return *this;
	}

	FILE_OFFSET& operator += (DWORD offset)
	{
		*((__int64*)this) += offset;
		return *this;
	}

	FILE_OFFSET& operator -= (DWORD offset)
	{
		*((__int64*)this) -= offset;
		return *this;
	}

	BOOL operator == (FILE_OFFSET& offset)
	{
		return (dwLow == offset.dwLow && dwHigh == offset.dwHigh);
	}

	BOOL operator != (FILE_OFFSET& offset)
	{
		return !(*this == offset);
	}
};

struct QueueNode
{
	struct QueueNode* pNext;
	UINT   message;
	SERIAL_STRUCT* pSerial;
};

struct READWRITE_STRUCT : public SERIAL_STRUCT
{
	int				iIndex : 8;
	bool			bBusy : 1;
	bool			bResult : 1;
	bool			bError : 1;
	bool			bWrite : 1;
	HANDLE			hFileHandle;
	LPCVOID			lpBuffer;
	DWORD			nNumberOfBytesToRead;
	LPDWORD			lpNumberOfBytesRead;
	LPOVERLAPPED	lpOverlapped;
	FILE_OFFSET		start_offset;
	QueueNode		queueNode;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct DACOM_NO_VTABLE DOSFileSystem : public IFileSystem
{
	char			debugTag[9];		// "DOSFile: "
	char			szFilename[MAX_PATH + 4];
	DWORD			dwAccess;            // The mode for the file
	DWORD			dwLastError;
	LPFILESYSTEM	pParent;
	HANDLE			hFile;
	DWORD			dwAllocationMask;		// related to page allocation granularity
	FILE_OFFSET		file_position;
	BOOL			bOpen;
	int				iRootIndex;			// point where non-root begins (index of last '\\'+1)

	READWRITE_STRUCT	operations[MAX_OVERLAPPED_OPS];
	CRITICAL_SECTION	criticalSection;
	int					numOperations;		// current number of read/write operations in progress


	BEGIN_DACOM_MAP_INBOUND(DOSFileSystem)
		DACOM_INTERFACE_ENTRY(IFileSystem)
		DACOM_INTERFACE_ENTRY2(IID_IFileSystem, IFileSystem)
		END_DACOM_MAP()

	//---------------------------
	// public methods
	//---------------------------

	void* operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void operator delete(void* ptr)
	{
		free(ptr);
	}

	DOSFileSystem(void)
	{
#ifdef DA_HEAP_ENABLED
		HEAP->SetBlockMessage(this, debugTag);
		memcpy(debugTag, "DOSFile: ", sizeof(debugTag));
#endif
		szFilename[0] = '\\';
		hFile = INVALID_HANDLE_VALUE;
		InitializeCriticalSection(&criticalSection);
	}

	~DOSFileSystem(void);

	// *** IComponentFactory methods ***

	DEFMETHOD(CreateInstance) (DACOMDESC* descriptor, void** instance);

	// *** IFileSystem methods ***

	DEFMETHOD_(BOOL, CloseHandle) (HANDLE handle = 0);

	DEFMETHOD_(BOOL, ReadFile) (HANDLE hFileHandle, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead,
		LPOVERLAPPED lpOverlapped);

	DEFMETHOD_(BOOL, WriteFile) (HANDLE hFileHandle, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

	DEFMETHOD_(BOOL, GetOverlappedResult)   (HANDLE hFileHandle,
		LPOVERLAPPED lpOverlapped,
		LPDWORD lpNumberOfBytesTransferred,
		BOOL bWait);

	DEFMETHOD_(DWORD, SetFilePointer) (HANDLE hFileHandle, LONG lDistanceToMove,
		PLONG lpDistanceToMoveHigh = 0, DWORD dwMoveMethod = FILE_BEGIN);

	DEFMETHOD_(BOOL, SetEndOfFile) (HANDLE hFileHandle = 0);

	DEFMETHOD_(DWORD, GetFileSize) (HANDLE hFileHandle, LPDWORD lpFileSizeHigh = 0);

	DEFMETHOD_(BOOL, LockFile) (HANDLE hFile,
		DWORD dwFileOffsetLow,
		DWORD dwFileOffsetHigh,
		DWORD nNumberOfBytesToLockLow,
		DWORD nNumberOfBytesToLockHigh);

	DEFMETHOD_(BOOL, UnlockFile) (HANDLE hFile,
		DWORD dwFileOffsetLow,
		DWORD dwFileOffsetHigh,
		DWORD nNumberOfBytesToUnlockLow,
		DWORD nNumberOfBytesToUnlockHigh);

	DEFMETHOD_(BOOL, GetFileTime) (HANDLE hFileHandle, LPFILETIME lpCreationTime,
		LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);

	DEFMETHOD_(BOOL, SetFileTime) (HANDLE hFileHandle, CONST FILETIME* lpCreationTime,
		CONST FILETIME* lpLastAccessTime,
		CONST FILETIME* lpLastWriteTime);

	DEFMETHOD_(HANDLE, CreateFileMapping)   (HANDLE hFileHandle,
		LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
		DWORD flProtect,
		DWORD dwMaximumSizeHigh,
		DWORD dwMaximumSizeLow,
		LPCTSTR lpName);

	DEFMETHOD_(LPVOID, MapViewOfFile)      (HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh,
		DWORD dwFileOffsetLow,
		DWORD dwNumberOfBytesToMap);

	DEFMETHOD_(BOOL, UnmapViewOfFile)      (LPCVOID lpBaseAddress);

	DEFMETHOD_(HANDLE, FindFirstFile) (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);

	DEFMETHOD_(BOOL, FindNextFile) (HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);

	DEFMETHOD_(BOOL, FindClose) (HANDLE hFindFile);

	DEFMETHOD_(BOOL, CreateDirectory) (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);

	DEFMETHOD_(BOOL, RemoveDirectory) (LPCTSTR lpPathName);

	DEFMETHOD_(DWORD, GetCurrentDirectory) (DWORD nBufferLength, LPTSTR lpBuffer);

	DEFMETHOD_(BOOL, SetCurrentDirectory) (LPCTSTR lpPathName);

	DEFMETHOD_(BOOL, DeleteFile)  (LPCTSTR lpFileName);

	DEFMETHOD_(BOOL, CopyFile)    (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists);

	DEFMETHOD_(BOOL, MoveFile) (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);

	DEFMETHOD_(DWORD, GetFileAttributes) (LPCTSTR lpFileName);

	DEFMETHOD_(BOOL, SetFileAttributes) (LPCTSTR lpFileName, DWORD dwFileAttributes);

	DEFMETHOD_(DWORD, GetLastError) (VOID);

	//---------------   
	// IFileSystem extensions to WIN32 system
	//---------------   

	DEFMETHOD_(HANDLE, OpenChild) (DAFILEDESC* lpDesc);

	DEFMETHOD_(DWORD, GetFilePosition) (HANDLE hFileHandle = 0, PLONG pPositionHigh = 0);

	DEFMETHOD_(LONG, GetFileName) (LPSTR lpBuffer, LONG lBufferSize);

	DEFMETHOD_(DWORD, GetAccessType) (VOID);

	DEFMETHOD(GetParentSystem) (LPFILESYSTEM* lplpFileSystem);

	DEFMETHOD(SetPreference)  (DWORD dwNumber, DWORD  dwValue);

	DEFMETHOD(GetPreference)  (DWORD dwNumber, PDWORD pdwValue);

	DEFMETHOD(ReadDirectoryExtension) (HANDLE hFile, LPVOID lpBuffer,
		DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead = 0, DWORD dwStartOffset = 0);

	DEFMETHOD(WriteDirectoryExtension) (HANDLE hFile, LPCVOID lpBuffer,
		DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten = 0, DWORD dwStartOffset = 0);

	DEFMETHOD_(LONG, SerialCall) (LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, VOID* lpContext);

	DEFMETHOD_(BOOL, GetAbsolutePath) (char* lpOutput, LPCTSTR lpInput, LONG lSize);

	//---------------   
	// DOSFileSystem methods
	//---------------   

	HANDLE TranslateHandle(HANDLE handle);

	SERIALMETHOD(Seek_S);

	SERIALMETHOD(StartReadWrite_S);

	SERIALMETHOD(ReadWrite_S);

	SERIALMETHOD(CloseAllHandles_S);
};

extern DOSFileSystem* pFirstSystem;
extern HINSTANCE hInstance;
extern HANDLE hEvent;
extern HANDLE hThread;
extern DWORD dwThreadID;
extern QueueNode* pMessageList;

extern "C"
{
#if ENABLE_DOS_THREADING
	READFILE_DEC BOOL StartUpFileSystem(void);
#endif // ENABLE_DOS_THREADING
	READFILE_DEC LONG __stdcall DOS__SerialCall(LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, VOID* lpContext);
	READFILE_DEC IFileSystem* CreateDOSFileSystem();
	READFILE_DEC void Register_DOSFileSystem();
}

#endif // __DOSFILESYSTEM_H__
