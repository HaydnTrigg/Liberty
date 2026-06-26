#pragma once

#include "ProfileParser.h"

struct DACOM_NO_VTABLE ProfileParser2 : public IProfileParser2, ProfileParser
{
	//
	// interface mapping
	//
	BEGIN_DACOM_MAP_INBOUND(ProfileParser2)
		DACOM_INTERFACE_ENTRY(IProfileParser2)
		DACOM_INTERFACE_ENTRY(IProfileParser)
		DACOM_INTERFACE_ENTRY2(IID_IProfileParser2, IProfileParser2)
		DACOM_INTERFACE_ENTRY2(IID_IProfileParser, IProfileParser)
		END_DACOM_MAP()


	GENRESULT init(PROFPARSEDESC2* info) { return GR_OK; }

	/* IProfileParser methods */

	DEFMETHOD(Initialize) (const C8* fileName, ACCESS access = READ_ACCESS) override
	{
		GENRESULT result = ProfileParser::Initialize(fileName, access);
		return result;
	}
	DEFMETHOD_(BOOL32, EnumerateSections) (ENUM_PROC proc = 0, void* context = 0) override
	{
		BOOL32 result = ProfileParser::EnumerateSections(proc, context);
		return result;
	}
	DEFMETHOD_(HANDLE, CreateSection) (const C8* sectionName, CREATE_MODE mode = PP_OPENEXISTING) override
	{
		HANDLE result = ProfileParser::CreateSection(sectionName, mode);
		return result;
	}
	DEFMETHOD_(BOOL32, CloseSection) (HANDLE hSection) override
	{
		BOOL32 result = ProfileParser::CloseSection(hSection);
		return result;
	}
	DEFMETHOD_(U32, ReadProfileLine) (HANDLE hSection, U32 lineNumber, C8* buffer, U32 bufferSize) override
	{
		U32 result = ProfileParser::ReadProfileLine(hSection, lineNumber, buffer, bufferSize);
		return result;
	}
	DEFMETHOD_(U32, ReadKeyValue) (HANDLE hSection, const C8* keyName, C8* buffer, U32 bufferSize) override
	{
		U32 result = ProfileParser::ReadKeyValue(hSection, keyName, buffer, bufferSize);
		return result;
	}

	/* IProfileParser2 methods */

	DEFMETHOD(Initialize2) (const C8* buffer, U32 bufferSize) override;
	DEFMETHOD_(BOOL32, EnumerateSections) (IProfileCallback* callback, void* context = 0) override;
	DEFMETHOD_(BOOL32, EnumerateKeys) (IProfileCallback* callback, HANDLE hSection, void* context = 0) override;
};
