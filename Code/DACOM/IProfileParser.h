#pragma once

//-----------------------------------------------------------------------------
//---------------------------IProfileParser interface--------------------------
//-----------------------------------------------------------------------------

#define IID_IProfileParser DACOM_MAKE_IID("IProfileParser")

struct PROFPARSEDESC : public AGGDESC
{
	PROFPARSEDESC(const C8* _interfaceName = IID_IProfileParser) : AGGDESC(_interfaceName)
	{
		size = sizeof(*this);
	};
};

struct DACOM_NO_VTABLE IProfileParser : public IDAComponent
{
	// enumeration callback routine - return TRUE to continue the enumeration

	typedef BOOL32(__stdcall* ENUM_PROC) (struct IProfileParser* parser,
		const C8* sectionName,
		void* context);

	enum ACCESS
	{
		READ_ACCESS = 0,
		WRITE_ACCESS
	};

	enum CREATE_MODE
	{
		PP_CREATENEW = 1,
		PP_CREATEALWAYS = 2,
		PP_OPENEXISTING = 3,
		PP_OPENALWAYS = 4,
		PP_TRUNCATEEXISTING = 5
	};

	DEFMETHOD(Initialize) (const C8* fileName, ACCESS access = READ_ACCESS) = 0;

	DEFMETHOD_(BOOL32, EnumerateSections) (ENUM_PROC proc = 0, void* context = 0) = 0;

	DEFMETHOD_(HANDLE, CreateSection) (const C8* sectionName, CREATE_MODE mode = PP_OPENEXISTING) = 0;

	DEFMETHOD_(BOOL32, CloseSection) (HANDLE hSection) = 0;

	DEFMETHOD_(U32, ReadProfileLine) (HANDLE hSection, U32 lineNumber, C8* buffer, U32 bufferSize) = 0;

	DEFMETHOD_(U32, ReadKeyValue) (HANDLE hSection, const C8* keyName, C8* buffer, U32 bufferSize) = 0;
};

//-----------------------------------------------------------------------------
//---------------------------IProfileParser2 interface-------------------------
//-----------------------------------------------------------------------------

#define IID_IProfileParser2 DACOM_MAKE_IID("IProfileParser2")

struct PROFPARSEDESC2 : public AGGDESC
{
	PROFPARSEDESC2(const C8* _interfaceName = IID_IProfileParser2) : AGGDESC(_interfaceName)
	{
		size = sizeof(*this);
	};
};

struct DACOM_NO_VTABLE IProfileCallback
{
	virtual BOOL32 __stdcall ProfileCallback(struct IProfileParser2* parser, const C8* name, const C8* value, void* context) = 0;
};

struct DACOM_NO_VTABLE IProfileParser2 : public IDAComponent
{
	DEFMETHOD(Initialize) (const C8* fileName, IProfileParser::ACCESS access = IProfileParser::READ_ACCESS) = 0;
	DEFMETHOD(Initialize2) (const C8* buffer, U32 bufferSize) = 0;
	DEFMETHOD_(HANDLE, CreateSection) (const C8* sectionName, IProfileParser::CREATE_MODE mode = IProfileParser::PP_OPENEXISTING) = 0;
	DEFMETHOD_(BOOL32, CloseSection) (HANDLE hSection) = 0;
	DEFMETHOD_(U32, ReadProfileLine) (HANDLE hSection, U32 lineNumber, C8* buffer, U32 bufferSize) = 0;
	DEFMETHOD_(U32, ReadKeyValue) (HANDLE hSection, const C8* keyName, C8* buffer, U32 bufferSize) = 0;
	DEFMETHOD_(BOOL32, EnumerateSections) (IProfileCallback* callback, void* context = 0) = 0;
	DEFMETHOD_(BOOL32, EnumerateKeys) (IProfileCallback* callback, HANDLE hSection, void* context = 0) = 0;
};
