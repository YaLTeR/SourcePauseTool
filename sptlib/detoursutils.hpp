#include "sptlib-stdafx.hpp"
#pragma once

void AttachDetours( const std::wstring &moduleName, unsigned int argCount, ... );
void DetachDetours( const std::wstring &moduleName, unsigned int argCount, ... );
