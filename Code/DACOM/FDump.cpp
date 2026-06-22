#include "DACOM.h"

#include <stdio.h>

static bool s_swappedFDUMP = false;
DA_ERROR_HANDLER COMMON_FDUMP = NULL;
int __cdecl CUSTOM_DA_ERROR_HANDLER(ErrorCode code, const C8* format, ...)
{
	if (!s_swappedFDUMP)
	{
		if (FDUMP == CUSTOM_DA_ERROR_HANDLER)
		{
			COMMON_FDUMP = FDUMP; // server
		}
		else
		{
			COMMON_FDUMP = FDUMP; // game
		}
		FDUMP = CUSTOM_DA_ERROR_HANDLER;
		s_swappedFDUMP = true;
	}

	va_list args;
	va_start(args, format);

	// Determine the length of the formatted string
	va_list args_copy;
	va_copy(args_copy, args);
	int length = vsnprintf(NULL, 0, format, args_copy);
	va_end(args_copy);

	// Allocate memory for the formatted string
	char* const buffer = (char*)malloc((length + 1) * sizeof(char)); // +1 for null terminator
	if (buffer != NULL) {
		vsprintf(buffer, format, args); // Format string into the buffer
	}

	// Trim whitespace from the end of the buffer
	{
		int len = strlen(buffer);
		while (len > 0 && isspace(buffer[len - 1])) {
			len--;
		}
		buffer[len] = '\0';
	}

	bool excluded = false;
	if (strstr(buffer, "C:\\work\\builds\\dalibs\\dalibs-build\\build\\Src\\WavLib\\wavlib.cpp") == buffer)
	{
		excluded = true;
	}

	if (strstr(buffer, "E:\\FL\\Scratch\\Source\\Client\\cmn\\Voice.cpp") == buffer)
	{
		debug_point;
	}

	if (!excluded)
	{
		printf("%s\n", buffer);
	}

	va_list args_copy2;
	va_copy(args_copy2, args);
	const char* filename = va_arg(args_copy2, const char*);
	int line = va_arg(args_copy2, int);
	int stack8 = va_arg(args_copy2, int);
	int stackC = va_arg(args_copy2, int);
	int stack10 = va_arg(args_copy2, int);
	int stack14 = va_arg(args_copy2, int);
	va_end(args_copy2);

	va_end(args); // Clean up the va_list

	int result = 0;
	if (COMMON_FDUMP != &CUSTOM_DA_ERROR_HANDLER)
	{
		result = COMMON_FDUMP(code, format, filename, line, stack8, stackC, stack10, stack14);
	}
	return result;
}

extern "C"
{
	typedef int(__cdecl* DA_ERROR_HANDLER) (ErrorCode code, const C8* fmt, ...);
	DACOM_DEC DA_ERROR_HANDLER FDUMP = CUSTOM_DA_ERROR_HANDLER;
}
