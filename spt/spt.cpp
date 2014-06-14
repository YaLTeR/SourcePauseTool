#include <string>

void ( *EngineLog )( const char *format, ... );
void ( *EngineDevLog )( const char *format, ... );
void ( *EngineWarning )( const char *format, ... );

std::wstring GetFileName( const std::wstring &fileNameWithPath )
{
	unsigned slashPos = fileNameWithPath.rfind('/');
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
