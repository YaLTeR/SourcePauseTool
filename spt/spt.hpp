#ifndef __SPT_H__
#define __SPT_H__

#ifdef _WIN32
#pragma once
#endif

#define SPT_VERSION "0.3-beta"

extern void ( *EngineLog )( const char *format, ... );
extern void ( *EngineDevLog )( const char *format, ... );
extern void ( *EngineWarning )( const char *format, ... );

#endif // __SPT_H__
