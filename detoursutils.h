#ifndef __DETOURSUTILS_H__
#define __DETOURSUTILS_H__

#ifdef _WIN32
#pragma once
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>

#include <detours.h>

void AttachDetours( const std::wstring &moduleName, unsigned int argCount, ... );
void DetachDetours( const std::wstring &moduleName, unsigned int argCount, ... );

#endif // __DETOURSUTILS_H__