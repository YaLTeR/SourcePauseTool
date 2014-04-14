#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>

#include <detours.h>
#include "detoursutils.h"
#include "spt.h"

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
        EngineLog( "No %s functions to detour!\n", WStringToString( moduleName ).c_str() );
        return;
    }

    DetourTransactionBegin();
    DetourUpdateThread( GetCurrentThread() );

    for (unsigned int i = 0; i < argCount; i += 2)
    {
        PVOID *pFunctionToDetour = va_arg( copy, PVOID * );
        PVOID functionToDetourWith = va_arg( copy, PVOID );

        if ((pFunctionToDetour && *pFunctionToDetour) && functionToDetourWith)
        {
            DetourAttach( pFunctionToDetour, functionToDetourWith );
        }
    }
    va_end( copy );

    LONG error = DetourTransactionCommit();
    if (error == NO_ERROR)
    {
        EngineLog( "Detoured the %s functions.\n", WStringToString( moduleName ).c_str() );
    }
    else
    {
        EngineWarning( "Error detouring the %s functions: %d.\n", WStringToString( moduleName ).c_str(), error );
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
    va_end( args );

    // We don't have anything to undetour.
    if (!needToUndetour)
    {
        va_end( copy );
        EngineLog( "No %s functions to detour!\n", WStringToString( moduleName ).c_str() );
        return;
    }

    DetourTransactionBegin();
    DetourUpdateThread( GetCurrentThread() );

    for (unsigned int i = 0; i < argCount; i += 2)
    {
        PVOID *pFunctionToUndetour = va_arg( copy, PVOID * );
        PVOID functionReplacement = va_arg( copy, PVOID );

        if ((pFunctionToUndetour && *pFunctionToUndetour) && functionReplacement)
        {
            DetourDetach( pFunctionToUndetour, functionReplacement );
        }
    }
    va_end( copy );

    LONG error = DetourTransactionCommit();
    if (error == NO_ERROR)
    {
        EngineLog( "Removed the %s function detours.\n", WStringToString( moduleName ).c_str() );
    }
    else
    {
        EngineWarning( "Error removing the %s function detours: %d.\n", WStringToString( moduleName ).c_str(), error );
    }
}