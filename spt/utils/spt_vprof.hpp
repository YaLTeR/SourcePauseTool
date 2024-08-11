#pragma once

/*
* A small header for using valve's profiler. This just makes it easy to include
* vprof.h with the 2007 & 2013 SDKs. The simplest way to use it is to add
* VPROF_BUDGET(__FUNCTION__, "MY_GROUP_NAME");
* to the function/scope that you want to profile. Then enable profiling with
* "vprof_on" (or toggle with "vprof"), then run "+showvprof".
*/

#define VPROF_LEVEL 1
#ifndef SSDK2007
#define RAD_TELEMETRY_DISABLED
#endif
#include "vprof.h"
