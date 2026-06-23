#pragma once

#ifndef __STREAMSOURCE_H__
#define __STREAMSOURCE_H__

#include <FileSys.h>

/*
 * StreamSource.h
 *
 * Thin wrapper around an IFileSystem-backed file. The SoundStreamer never
 * touches the Win32 file API directly; all reads are routed through the engine's
 * virtual file system so that streamed audio works equally well whether it lives
 * on disk or inside a packaged archive.
 *
 * Two open modes are supported, matching the original component:
 *   - If the caller supplies a parent file system, the file is opened as a child
 *     of it (IFileSystem::OpenChild) and reads use the returned handle.
 *   - Otherwise a file system is created on demand from the DACOM component
 *     manager, and reads use that system's own (default) handle.
 */
class StreamSource
{
public:
	StreamSource() = default;
	~StreamSource();

	StreamSource(const StreamSource&) = delete;
	StreamSource& operator=(const StreamSource&) = delete;

	/*
	 * Opens 'filename' for reading. When 'parent' is non-null the file is opened
	 * relative to it; otherwise the default file system is used. Returns true on
	 * success.
	 */
	bool Open(const char* filename, IFileSystem* parent);

	// Closes the file and releases the underlying file system reference.
	void Close();

	/*
	 * Reads up to 'bytes' bytes into 'buffer'. Returns the number of bytes
	 * actually read (0 at end-of-file or on error).
	 */
	DWORD Read(void* buffer, DWORD bytes);

	// Moves the read cursor to an absolute byte offset. Returns true on success.
	bool Seek(LONG offset);

	// Returns the total size of the file, in bytes.
	DWORD Size();

	bool IsOpen() const { return m_fileSystem != nullptr; }

private:
	IFileSystem* m_fileSystem = nullptr;	// File system the file is read through.
	HANDLE m_handle = nullptr;				// File handle; null means "the file system's own file".
	bool m_ownsFileSystem = false;			// True if we created m_fileSystem and must release it.
	bool m_ownsHandle = false;				// True if m_handle came from OpenChild and must be closed.
};

#endif // __STREAMSOURCE_H__
