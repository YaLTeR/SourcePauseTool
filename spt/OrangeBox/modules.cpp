#include "modules\ClientDLL.hpp"
#include "modules\EngineDLL.hpp"
#include "modules\inputSystemDLL.hpp"
#include "modules\ServerDLL.hpp"
#include "modules\vguimatsurfaceDLL.hpp"

EngineDLL engineDLL;
ClientDLL clientDLL;
ServerDLL serverDLL;
#ifndef OE
VGui_MatSurfaceDLL vgui_matsurfaceDLL;
#endif
InputSystemDLL inputSystemDLL;