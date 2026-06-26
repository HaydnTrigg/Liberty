#pragma once

#ifndef __BASEUTF_H__
#define __BASEUTF_H__
#ifdef UTF_FILESYSTEM

#include "FileSys.h"

#ifndef MAKE_4CHAR
#define MAKE_4CHAR(a,b,c,d) (((U32) (a)) + (((U32) (b)) << 8) + (((U32) (c)) << 16) + (((U32) (d)) << 24))
#endif

// options that can be switched over to preferences later

#define UTF_SWITCH_CHAR '\\'
#define UTF_EXTRA_ALLOC(x) (0) // extra space to alloc when allocating a new block of length 'x'
#define DEFAULT_UTF_EXTRA_ENTRIES 0
#define DEFAULT_UTF_EXTRA_NAME_SPACE 0

#define UTF_VERSION 0x101

#define UTF_IMPLEMENTATION_NAME "UTF"

#define FILE_ATTRIBUTE_UNUSED 0xFFFFFFFF // unused directory entry

// get handle from DAFILEDESC, backward compatible
#define GETFFHANDLE(x) (x->size==sizeof(DAFILEDESC) ? x->hFindFirst : INVALID_HANDLE_VALUE)

// UTF container file header
struct UTF_HEADER
{
	DWORD dwIdentifier; // 'UTF ' identifier
	DWORD dwVersion; // File version number (0x100 = 1.00)

	DWORD dwDirectoryOffset; // Offset to beginning of directory area
	DWORD dwDirectorySize; // Size of directory area in bytes (used area)

	DWORD dwUnusedEntryOffset; // offset to first unused entry, 0 == nothing unused
	DWORD dwDirEntrySize; // size of a directory entry, in bytes

	DWORD dwNamesOffset; // offset to buffer containing all of the ASCIIZ filenames
	DWORD dwNameSpaceSize; // size of the names buffer
	DWORD dwNameSpaceUsed; // number of bytes actually used in the names buffer

	DWORD dwDataStartOffset; // offset to beginning of file data

	DWORD dwUnusedSpaceOffset; // offset to first block of unused space
	DWORD dwUnusedSpaceSize; // Cumulative size of unused resources

	FILETIME LastWriteTime; // last time directory structure was changed

	// Reserved expansion area for future header versions
};

// read + write may exceed the actual number of files open if some files have read+write access
// the same goes for readSharing and writeSharing
struct UTF_SHARING
{
	byte read; // count of files open with read access
	byte write; // count of files open with write access
	byte readSharing; // count of files open with read sharing on
	byte writeSharing; // count of files open with write sharing on

	UTF_SHARING& operator += (const UTF_SHARING& access)
	{
		read += access.read;
		write += access.write;
		if (access.readSharing)
			readSharing += access.read + access.write;
		if (access.writeSharing)
			writeSharing += access.read + access.write;
		return *this;
	}

	UTF_SHARING& operator -= (const UTF_SHARING& access)
	{
		read -= access.read;
		write -= access.write;
		if (access.readSharing)
			readSharing -= access.read + access.write;
		if (access.writeSharing)
			writeSharing -= access.read + access.write;
		return *this;
	}

	// returns TRUE if proposed access meets the current sharing environment
	BOOL isCompatible(const UTF_SHARING& access);

	BOOL isEmpty(void)
	{
		return read == 0 && write == 0 && readSharing == 0 && writeSharing == 0;
	}

	void setEmpty(void)
	{
		read = 0;
		write = 0;
		readSharing = 0;
		writeSharing = 0;
	}
};

// UTF directory entry representation
struct UTF_DIR_ENTRY
{
	DWORD dwNext; // offset to next entry within directory
	DWORD dwName; // offset into name buffer of an ASCIIZ string

	DWORD dwAttributes; // directory / file / read-only, etc
	UTF_SHARING Sharing; // dynamic sharing state

	DWORD dwDataOffset; // file data (for file), or directory offset to child directory entry
	DWORD dwSpaceAllocated; // disk space allocated for file (may be larger than used)
	DWORD dwSpaceUsed; // disk space actually used by the file
	DWORD dwUncompressedSize; // uncompressed file size

	DWORD DOSCreationTime;
	DWORD DOSLastAccessTime;
	DWORD DOSLastWriteTime;
};

struct UTF_READ_STRUCT
{
	HANDLE hFileHandle;
	LPVOID lpBuffer;
	DWORD nNumberOfBytesToRead;
	LPDWORD lpNumberOfBytesRead;
	LPOVERLAPPED lpOverlapped;
};

struct UTF_WRITE_STRUCT
{
	HANDLE hFileHandle;
	LPCVOID lpBuffer;
	DWORD nNumberOfBytesToWrite;
	LPDWORD lpNumberOfBytesWritten;
	LPOVERLAPPED lpOverlapped;
};

// define the basic version of UTF implementation

struct DACOM_NO_VTABLE BaseUTF : public IFileSystem
{
	char szFilename[MAX_PATH + 4];
	DWORD dwAccess; // The mode for the file
	DWORD dwLastError;
	LPFILESYSTEM pParent;
	int iRootIndex; // point where non-root begins (index of last '\\'+1)

	HANDLE hParentFile;

	// data needed to optimize directory searches
	BaseUTF* pParentUTF; // may be NULL if parent if not UTF instance
	UTF_DIR_ENTRY* pBaseDirEntry; // will be valid only if pParentUTF is valid
	// points to current directory 

   // interface map
	BEGIN_DACOM_MAP_INBOUND(BaseUTF)
		DACOM_INTERFACE_ENTRY(IFileSystem)
		DACOM_INTERFACE_ENTRY2(IID_IFileSystem, IFileSystem)
		END_DACOM_MAP()

	// public methods

	BaseUTF(void)
	{
		szFilename[0] = '\\';
		hParentFile = INVALID_HANDLE_VALUE;
	}

	DA_HEAP_DEFINE_NEW_OPERATOR(BaseUTF);

	virtual ~BaseUTF(void);

	// *** IComponentFactory methods ***

	DEFMETHOD(CreateInstance) (DACOMDESC* descriptor, void** instance);

	// *** IFileSystem methods ***

	DEFMETHOD_(BOOL, CloseHandle) (HANDLE handle);
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
	DEFMETHOD_(BOOL, CopyFile) (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists);
	DEFMETHOD_(BOOL, MoveFile) (LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);
	DEFMETHOD_(DWORD, GetFileAttributes) (LPCTSTR lpFileName);
	DEFMETHOD_(BOOL, SetFileAttributes) (LPCTSTR lpFileName, DWORD dwFileAttributes);
	DEFMETHOD_(DWORD, GetLastError) (VOID);

	// IFileSystem extensions to WIN32 system

	DEFMETHOD_(HANDLE, OpenChild) (DAFILEDESC* lpDesc);
	DEFMETHOD_(DWORD, GetFilePosition) (HANDLE hFileHandle = 0, PLONG pPositionHigh = 0);
	DEFMETHOD_(LONG, GetFileName) (LPSTR lpBuffer, LONG lBufferSize);
	DEFMETHOD_(DWORD, GetAccessType) (VOID);
	DEFMETHOD(GetParentSystem) (LPFILESYSTEM* lplpFileSystem);
	DEFMETHOD(SetPreference) (DWORD dwNumber, DWORD dwValue);
	DEFMETHOD(GetPreference) (DWORD dwNumber, PDWORD pdwValue);
	DEFMETHOD(ReadDirectoryExtension) (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead = 0, DWORD dwStartOffset = 0);
	DEFMETHOD(WriteDirectoryExtension) (HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten = 0, DWORD dwStartOffset = 0);
	DEFMETHOD_(LONG, SerialCall) (LPFILESYSTEM lpSystem, DAFILE_SERIAL_PROC lpProc, VOID* lpContext);
	DEFMETHOD_(BOOL, GetAbsolutePath) (char* lpOutput, LPCTSTR lpInput, LONG lSize);

	// BaseUTF methods

	virtual BOOL init(DAFILEDESC* lpDesc); // return TRUE if successful
	static BOOL __stdcall GetDirectoryEntry(LPCSTR lpFileName, UTF_DIR_ENTRY* pDirectory, LPCTSTR pNames, UTF_DIR_ENTRY** ppEntry);
	static BOOL __fastcall DOSTimeToFileTime(DWORD dwDOSTime, FILETIME* pFileTime);
	static DWORD __fastcall FileTimeToDOSTime(CONST FILETIME* pFileTime);
	static DWORD __stdcall FindName(LPCTSTR lpFileName, LPCTSTR pNames);
	static bool __stdcall TestValid(LPCTSTR lpFileName); // return FALSE if name contains invalid characters

	// the following are only really implemented by the UTF implementation

	virtual HANDLE openChild(DAFILEDESC* lpDesc, UTF_DIR_ENTRY* pEntry);
	virtual DWORD getFileAttributes(LPCTSTR lpFileName, UTF_DIR_ENTRY* pEntry);
	virtual HANDLE findFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, UTF_DIR_ENTRY* pEntry);
	virtual UTF_DIR_ENTRY* getDirectoryEntryForChild(LPCSTR lpFileName, UTF_DIR_ENTRY* pRootDir = 0, HANDLE hFindFirst = INVALID_HANDLE_VALUE);
	virtual const char* getNameBuffer(void);
};

extern "C"
{
	READFILE_DEC IFileSystem* CreateBaseUTF();
	READFILE_DEC void Register_BaseUTF();
}

#endif // UTF_FILESYSTEM
#endif // __BASEUTF_H__
