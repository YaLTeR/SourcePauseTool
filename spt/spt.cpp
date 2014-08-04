#include <string>

void ( *EngineMsg )( const char *format, ... );
void ( *EngineDevMsg )( const char *format, ... );
void ( *EngineWarning )( const char *format, ... );
void ( *EngineDevWarning )( const char *format, ... );
void ( *EngineConCmd )( const char *cmd );

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
