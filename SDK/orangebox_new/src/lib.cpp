static void color_wrapper(int level, const Color& c, const char* msg, ...) {
	va_list a_list;
	va_start(a_list, msg);
	vprintf(msg, a_list);
	va_end(a_list);
}

static void printf_wrapper(const char* msg, ...) {
	va_list a_list;
	va_start(a_list, msg);
	vprintf(msg, a_list);
	va_end(a_list);
}

ColorFunc ConColorMsg = color_wrapper;
PrintFunc ConMsg = printf_wrapper;
PrintFunc Msg = printf_wrapper;
PrintFunc Warning = printf_wrapper;
PrintFunc DevMsg = printf_wrapper;
PrintFunc DevWarning = printf_wrapper;
extern "C" IMemAlloc* g_pMemAlloc = nullptr;
KeyValueSystemFunc KeyValuesSystem_impl = nullptr;

IKeyValuesSystem* KeyValuesSystem() {
	return KeyValuesSystem_impl();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : module - windows HMODULE from Sys_LoadModule() 
//			*pName - proc name
// Output : factory for this module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactory(CSysModule* pModule)
{
	if (!pModule)
		return NULL;

	HMODULE	hDLL = reinterpret_cast<HMODULE>(pModule);
	return reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hDLL, CREATEINTERFACE_PROCNAME));
}

static void* Sys_GetProcAddress(const char* pModuleName, const char* pName)
{
	HMODULE hModule = GetModuleHandle(pModuleName);
	return GetProcAddress(hModule, pName);
}
CreateInterfaceFn Sys_GetFactory(const char* pModuleName)
{
#ifdef _WIN32
	return static_cast<CreateInterfaceFn>(Sys_GetProcAddress(pModuleName, CREATEINTERFACE_PROCNAME));
#elif defined(_LINUX)
	// see Sys_GetFactory( CSysModule *pModule ) for an explanation
	return (CreateInterfaceFn)(Sys_GetProcAddress(pModuleName, CREATEINTERFACE_PROCNAME));
#endif
}
