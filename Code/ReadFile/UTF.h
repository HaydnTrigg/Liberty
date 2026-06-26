#pragma once

// READ-ONLY, and NON-SHARED instance of a UTF file system

#ifndef __UTF_H__
#define __UTF_H__
#ifdef UTF_FILESYSTEM

#include "BaseUTF.h"

#define MAX_GROUP_HANDLES 4
#define MEMORY_MAP_FLAG 0x40000000

struct UTF_CHILD
{
	UTF_DIR_ENTRY* pEntry;
	DWORD dwFilePosition;
};

struct UTF_CHILD_GROUP
{
	struct UTF_CHILD_GROUP* pNext;
	UTF_CHILD array[MAX_GROUP_HANDLES];

	void* operator new (size_t size)
	{
		return GlobalAlloc(GPTR, size);
	}

	void operator delete (void* p)
	{
		GlobalFree(p);
	}
};

struct UTF_FFHANDLE
{
	UTF_DIR_ENTRY* pEntry; // entry to examine on next call to FindNextFile()
	UTF_DIR_ENTRY* pPrevEntry; // entry last examined by call to FindFirst/FindNext
	char szPattern[MAX_PATH + 4];
};

struct UTF_FFGROUP
{
	struct UTF_FFGROUP* pNext;
	UTF_FFHANDLE array[MAX_GROUP_HANDLES];

	void* operator new (size_t size)
	{
		return GlobalAlloc(GPTR, size);
	}

	void operator delete (void* p)
	{
		GlobalFree(p);
	}
};

struct DACOM_NO_VTABLE UTF : public BaseUTF
{
	// root directory entry pertains to the whole file (size=size of file, start = 0)

	HANDLE hMapping; // mapping created by user
	UTF_DIR_ENTRY* pDirectory; // pointer to root directory instance
	LPCTSTR pNames; // pointer to the names buffer
	DWORD dwDataStartOffset; // offset of file data in this file system
	UTF_CHILD_GROUP children;
	UTF_FFGROUP ffhandles;
	char szPathname[MAX_PATH + 4];

	// public methods

	DA_HEAP_DEFINE_NEW_OPERATOR(UTF);
	UTF(void);
	virtual ~UTF(void);

	// *** IFileSystem methods ***

	DEFMETHOD_(BOOL, CloseHandle) (HANDLE handle = 0);
	DEFMETHOD_(BOOL, ReadFile) (HANDLE hFileHandle, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
	DEFMETHOD_(BOOL, WriteFile) (HANDLE hFileHandle, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
	DEFMETHOD_(BOOL, GetOverlappedResult) (HANDLE hFileHandle, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);
	DEFMETHOD_(DWORD, SetFilePointer) (HANDLE hFileHandle, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh = 0, DWORD dwMoveMethod = FILE_BEGIN);
	DEFMETHOD_(BOOL, SetEndOfFile) (HANDLE hFileHandle = 0);
	DEFMETHOD_(DWORD, GetFileSize) (HANDLE hFileHandle, LPDWORD lpFileSizeHigh = 0);
	DEFMETHOD_(BOOL, LockFile) (HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh);
	DEFMETHOD_(BOOL, UnlockFile) (HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh);
	DEFMETHOD_(BOOL, GetFileTime) (HANDLE hFileHandle, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
	DEFMETHOD_(BOOL, SetFileTime) (HANDLE hFileHandle, CONST FILETIME* lpCreationTime, CONST FILETIME* lpLastAccessTime, CONST FILETIME* lpLastWriteTime);
	DEFMETHOD_(HANDLE, CreateFileMapping) (HANDLE hFileHandle, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName);
	DEFMETHOD_(LPVOID, MapViewOfFile) (HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, DWORD dwNumberOfBytesToMap);
	DEFMETHOD_(BOOL, UnmapViewOfFile) (LPCVOID lpBaseAddress);
	DEFMETHOD_(HANDLE, FindFirstFile) (LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
	DEFMETHOD_(BOOL, FindNextFile) (HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
	DEFMETHOD_(BOOL, FindClose) (HANDLE hFindFile);
	DEFMETHOD_(BOOL, CreateDirectory) (LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
	DEFMETHOD_(BOOL, RemoveDirectory) (LPCTSTR lpPathName);
	DEFMETHOD_(DWORD, GetCurrentDirectory) (DWORD nBufferLength, LPTSTR lpBuffer);
	DEFMETHOD_(BOOL, SetCurrentDirectory) (LPCTSTR lpPathName);
	DEFMETHOD_(BOOL, DeleteFile) (LPCTSTR lpFileName);
	DEFMETHOD_(BOOL, MoveFile) (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);
	DEFMETHOD_(DWORD, GetFileAttributes) (LPCTSTR lpFileName);
	DEFMETHOD_(BOOL, SetFileAttributes) (LPCTSTR lpFileName, DWORD dwFileAttributes);

	// IFileSystem extensions to WIN32 system

	DEFMETHOD_(HANDLE, OpenChild) (DAFILEDESC* lpDesc);
	DEFMETHOD_(DWORD, GetFilePosition) (HANDLE hFileHandle = 0, PLONG pPositionHigh = 0);
	DEFMETHOD_(BOOL, GetAbsolutePath) (char* lpOutput, LPCTSTR lpInput, LONG lSize);

	// BaseUTF methods

	virtual BOOL init(DAFILEDESC* lpDesc); // return TRUE if successful
	virtual HANDLE openChild(DAFILEDESC* lpDesc, UTF_DIR_ENTRY* pEntry);
	virtual DWORD getFileAttributes(LPCTSTR lpFileName, UTF_DIR_ENTRY* pEntry);
	virtual UTF_DIR_ENTRY* getDirectoryEntryForChild(LPCSTR lpFileName, UTF_DIR_ENTRY* pRootDir, HANDLE hFindFirst);
	virtual HANDLE findFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, UTF_DIR_ENTRY* pEntry);
	virtual const char* getNameBuffer(void);

	// UTF methods

	UTF_CHILD* __fastcall GetUTFChild(HANDLE handle); // warning!! this will return bogus number if (handle == 0)
	UTF_FFHANDLE* __fastcall GetFF(HANDLE handle);
	DWORD GetStartOffset(HANDLE handle);
	BOOL32 __fastcall isValidHandle(HANDLE handle);
	HANDLE allocHandle(UTF_DIR_ENTRY* pEntry); // pEntry is the initial value
	HANDLE allocFFHandle(UTF_DIR_ENTRY* pEntry, const char* pattern); // pEntry is the initial value

	//--------------- 
	// serial methods
	//--------------- 

	SERIALMETHOD(CreateMapping_S);
	SERIALMETHOD(ReadFile_S);

	static CRITICAL_SECTION criticalSection;
};

extern "C"
{
	READFILE_DEC BaseUTF* CreateUTF(void);
	READFILE_DEC void startupUTF(void);
	READFILE_DEC void shutdownUTF(void);
}

#endif // UTF_FILESYSTEM
#endif // __UTF_H__
