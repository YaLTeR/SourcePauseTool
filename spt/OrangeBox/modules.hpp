#pragma once

#include "modules\EngineDLL.hpp"
#include "modules\ClientDLL.hpp"
#include "modules\ServerDLL.hpp"
#include "modules\vguimatsurfaceDLL.hpp"

extern EngineDLL engineDLL;
extern ClientDLL clientDLL;
extern ServerDLL serverDLL;
#ifndef OE
extern VGui_MatSurfaceDLL vgui_matsurfaceDLL;
#endif

#ifdef TRACE
#define TRACE_MSG(X) DevMsg(X)
#else
#define TRACE_MSG(X)
#endif

#define TRACE_ENTER() TRACE_MSG("ENTER:" __func__ "\n")
#define TRACE_EXIT() TRACE_MSG("EXIT:" __func__ "\n")