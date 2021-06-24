#pragma once

#include "modules\ClientDLL.hpp"
#include "modules\EngineDLL.hpp"
#include "modules\inputSystemDLL.hpp"
#include "modules\ServerDLL.hpp"
#include "modules\vguimatsurfaceDLL.hpp"
#include "modules\vphysicsDLL.hpp"

extern EngineDLL engineDLL;
extern ClientDLL clientDLL;
extern InputSystemDLL inputSystemDLL;
extern ServerDLL serverDLL;
extern VPhysicsDLL vphysicsDLL;
#ifndef OE
extern VGui_MatSurfaceDLL vgui_matsurfaceDLL;
#endif

#ifdef TRACE
#define TRACE_MSG(X) DevMsg(X)
#else
#define TRACE_MSG(X)
#endif

#define TRACE_ENTER() TRACE_MSG("ENTER: " __FUNCTION__ "\n")
#define TRACE_EXIT() TRACE_MSG("EXIT: " __FUNCTION__ "\n")
