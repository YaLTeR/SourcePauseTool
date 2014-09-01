#pragma once

#include <codecvt>
#include <locale>
#include <string>

#define SPT_VERSION "0.7-beta"

extern void ( *EngineMsg )( const char *format, ... );
extern void ( *EngineDevMsg )( const char *format, ... );
extern void ( *EngineWarning )( const char *format, ... );
extern void ( *EngineDevWarning )( const char *format, ... );
extern void ( *EngineConCmd )( const char *cmd );
extern void ( *EngineGetViewAngles )( float viewangles[3] );
extern void ( *EngineSetViewAngles )( const float viewangles[3] );

extern std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> string_converter;

std::wstring GetFileName( const std::wstring &fileNameWithPath );
