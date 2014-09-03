#define SPT_MESSAGE_PREFIX "SPT: "
#include <SPTLib\sptlib.hpp>

extern void( *EngineConCmd )( const char *cmd );
extern void( *EngineGetViewAngles )( float viewangles[3] );
extern void( *EngineSetViewAngles )( const float viewangles[3] );
