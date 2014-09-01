#include <codecvt>
#include <locale>
#include <string>

void ( *EngineMsg )( const char *format, ... );
void ( *EngineDevMsg )( const char *format, ... );
void ( *EngineWarning )( const char *format, ... );
void ( *EngineDevWarning )( const char *format, ... );
void ( *EngineConCmd )( const char *cmd );
void ( *EngineGetViewAngles )( float viewangles[3] );
void ( *EngineSetViewAngles )( const float viewangles[3] );

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> string_converter;

std::wstring GetFileName( const std::wstring &fileNameWithPath )
{
	size_t slashPos = fileNameWithPath.rfind('/');
	if (slashPos != std::wstring::npos)
	{
		return std::wstring( fileNameWithPath, (slashPos + 1) );
	}
	else
	{
		slashPos = fileNameWithPath.rfind('\\');
		if (slashPos != std::wstring::npos)
		{
			return std::wstring( fileNameWithPath, (slashPos + 1) );
		}
	}

	return fileNameWithPath;
}
