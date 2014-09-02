#include "stdafx.h"

#include <codecvt>
#include <locale>

#include <detours.h>
#include "detoursutils.hpp"
#include "sptlib.hpp"

#pragma comment (lib, "detours.lib")

void AttachDetours( const std::wstring &moduleName, unsigned int argCount, ... )
{
	if ((argCount < 2) || ((argCount % 2) != 0)) // Must pass a set of functions and a set of replacement functions.
		return;

	va_list args, copy;
	va_start( args, argCount );
	va_copy( copy, args );

	// Check if we need to detour something.
	bool needToDetour = false;
	for (unsigned int i = 0; i < argCount; i += 2)
	{
		PVOID *pFunctionToDetour = va_arg( args, PVOID * );
		PVOID functionToDetourWith = va_arg( args, PVOID );

		if ((pFunctionToDetour && *pFunctionToDetour) && functionToDetourWith)
		{
			// We have something to detour!
			needToDetour = true;
			break;
		}
	}
	va_end( args );

	// We don't have anything to detour.
	if (!needToDetour)
	{
		va_end( copy );
		EngineDevMsg("No %s functions to detour!\n", string_converter.to_bytes(moduleName).c_str());
		return;
	}

	DetourTransactionBegin();
	DetourUpdateThread( GetCurrentThread() );

	unsigned int detourCount = 0;
	for (unsigned int i = 0; i < argCount; i += 2)
	{
		PVOID *pFunctionToDetour = va_arg( copy, PVOID * );
		PVOID functionToDetourWith = va_arg( copy, PVOID );

		if ((pFunctionToDetour && *pFunctionToDetour) && functionToDetourWith)
		{
			DetourAttach( pFunctionToDetour, functionToDetourWith );
			detourCount++;
		}
	}
	va_end( copy );

	LONG error = DetourTransactionCommit();
	if (error == NO_ERROR)
	{
		EngineDevMsg("Detoured %d %s function(s).\n", detourCount, string_converter.to_bytes(moduleName).c_str());
	}
	else
	{
		EngineWarning("Error detouring %d %s function(s): %d.\n", detourCount, string_converter.to_bytes(moduleName).c_str(), error);
	}
}

void DetachDetours( const std::wstring &moduleName, unsigned int argCount, ... )
{
	if ((argCount < 2) || ((argCount % 2) != 0)) // Must pass a set of functions and a set of replacement functions.
		return;

	va_list args, copy;
	va_start( args, argCount );
	va_copy( copy, args );

	// Check if we need to undetour something.
	bool needToUndetour = false;
	for (unsigned int i = 0; i < argCount; i += 2)
	{
		PVOID *pFunctionToUndetour = va_arg( args, PVOID * );
		PVOID functionReplacement = va_arg( args, PVOID );

		if ((pFunctionToUndetour && *pFunctionToUndetour) && functionReplacement)
		{
			// We have something to detour!
			needToUndetour = true;
			break;
		}
	}
	va_end(args);

	// We don't have anything to undetour.
	if (!needToUndetour)
	{
		va_end( copy );
		EngineDevMsg("No %s functions to undetour!\n", string_converter.to_bytes(moduleName).c_str());
		return;
	}

	DetourTransactionBegin();
	DetourUpdateThread( GetCurrentThread() );

	unsigned int detourCount = 0;
	for (unsigned int i = 0; i < argCount; i += 2)
	{
		PVOID *pFunctionToUndetour = va_arg( copy, PVOID * );
		PVOID functionReplacement = va_arg( copy, PVOID );

		if ((pFunctionToUndetour && *pFunctionToUndetour) && functionReplacement)
		{
			DetourDetach( pFunctionToUndetour, functionReplacement );
			detourCount++;
		}
	}
	va_end( copy );

	LONG error = DetourTransactionCommit();
	if (error == NO_ERROR)
	{
		EngineDevMsg("Removed %d %s function detour(s).\n", detourCount, string_converter.to_bytes(moduleName).c_str());
	}
	else
	{
		EngineWarning("Error removing %d %s function detour(s): %d.\n", detourCount, string_converter.to_bytes(moduleName).c_str(), error);
	}
}
