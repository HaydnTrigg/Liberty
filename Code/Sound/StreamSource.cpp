#include "StreamSource.h"

#include <DACOM.h>

/*
 * StreamSource.cpp
 *
 * Implementation of the IFileSystem-backed file reader. See StreamSource.h for
 * the contract.
 */

StreamSource::~StreamSource()
{
	Close();
}

/*
 * Opens 'filename' for reading. With a parent file system the file is opened as a
 * child of it; without one, a file system bound to the file is created from the
 * component manager. Returns true on success.
 */
bool StreamSource::Open(const char* filename, IFileSystem* parent)
{
	// Re-opening is not supported; close any previous file first.
	Close();

	if (filename == nullptr)
	{
		return false;
	}

	// DAFILEDESC defaults to GENERIC_READ / FILE_SHARE_READ / OPEN_EXISTING with
	// sequential-scan hinting, which is exactly what a streamer wants.
	DAFILEDESC descriptor(filename);

	if (parent != nullptr)
	{
		// Open the file as a child of the supplied file system.
		m_handle = parent->OpenChild(&descriptor);
		if (m_handle == INVALID_HANDLE_VALUE)
		{
			m_handle = nullptr;
			return false;
		}

		m_fileSystem = parent;
		m_fileSystem->AddRef();
		m_ownsFileSystem = false;
		m_ownsHandle = true;
		return true;
	}

	// No parent supplied: ask the component manager to create a file system bound
	// to the requested file. Reads then operate on its default handle.
	ICOManager* manager = DACOM_Acquire();
	if (manager == nullptr)
	{
		return false;
	}

	if (manager->CreateInstance(&descriptor, reinterpret_cast<void**>(&m_fileSystem)) != GR_OK || m_fileSystem == nullptr)
	{
		m_fileSystem = nullptr;
		return false;
	}

	m_handle = nullptr;			// Use the file system's own (default) file handle.
	m_ownsFileSystem = true;
	m_ownsHandle = false;
	return true;
}

/*
 * Closes the child handle (if any) and releases the file system reference.
 */
void StreamSource::Close()
{
	if (m_fileSystem != nullptr)
	{
		if (m_ownsHandle && m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE)
		{
			m_fileSystem->CloseHandle(m_handle);
		}

		m_fileSystem->Release();
		m_fileSystem = nullptr;
	}

	m_handle = nullptr;
	m_ownsFileSystem = false;
	m_ownsHandle = false;
}

/*
 * Reads up to 'bytes' bytes into 'buffer'. Returns the number of bytes read, or 0
 * at end-of-file or on error.
 */
DWORD StreamSource::Read(void* buffer, DWORD bytes)
{
	if (m_fileSystem == nullptr || buffer == nullptr || bytes == 0)
	{
		return 0;
	}

	DWORD bytesRead = 0;
	if (!m_fileSystem->ReadFile(m_handle, buffer, bytes, &bytesRead, nullptr))
	{
		return 0;
	}

	return bytesRead;
}

/*
 * Moves the read cursor to absolute byte offset 'offset'. Returns true on
 * success.
 */
bool StreamSource::Seek(LONG offset)
{
	if (m_fileSystem == nullptr)
	{
		return false;
	}

	return m_fileSystem->SetFilePointer(m_handle, offset, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
}

/*
 * Returns the total size of the file, in bytes (0 if not open).
 */
DWORD StreamSource::Size()
{
	if (m_fileSystem == nullptr)
	{
		return 0;
	}

	return m_fileSystem->GetFileSize(m_handle);
}
