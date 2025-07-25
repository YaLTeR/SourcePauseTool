#pragma once
#include "interface.h"
#include <array>
#include <vector>
#include <functional>
#include <unordered_map>
#include "SPTLib\patterns.hpp"
#include "SPTLib\memutils.hpp"
#include "convar.hpp"

/*
* Prefer using DECL_STATIC_HOOK_XXX over DECL_HOOK_XXX.
* The difference is that with the former you can do:
* 
* IMPL_HOOK_XXX(SptFeature, void, GameFn, int arg1) {
*   ORIG_GameFn(arg1);
* }
* 
* The DECL_HOOK_XXX macros keep the ORIG pointer as a
* member, so you're forced to do:
* 
* IMPL_HOOK_XXX(SptFeature, void, GameFn, int arg1) {
*   spt_feature_instance.ORIG_GameFn(arg1);
* }
*/

#if defined(_MSC_VER) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)

#define _DECL_FN(qualifiers, call_conv, retType, name, ...) \
	using _##name = retType(call_conv*)(##__VA_ARGS__); \
	qualifiers _##name ORIG_##name = nullptr

#define _DECL_MEMBER_FN(call_conv, retType, name, ...) _DECL_FN(, call_conv, retType, name, ##__VA_ARGS__)

#define _DECL_HOOK_FN(call_conv, retType, name, ...) \
	_DECL_MEMBER_FN(call_conv, retType, name, ##__VA_ARGS__); \
	static retType call_conv HOOKED_##name(__VA_ARGS__)

#define _DECL_STATIC_FN(call_conv, retType, name, ...) _DECL_FN(inline static, call_conv, retType, name, ##__VA_ARGS__)

#define _DECL_STATIC_HOOK_FN(call_conv, retType, name, ...) \
	_DECL_STATIC_FN(call_conv, retType, name, ##__VA_ARGS__); \
	static retType call_conv HOOKED_##name(__VA_ARGS__)

#define _IMPL_HOOK_FN(call_conv, spt_class, retType, name, ...) \
	retType call_conv spt_class::HOOKED_##name(##__VA_ARGS__)

// cdecl

#define DECL_MEMBER_CDECL(retType, name, ...) _DECL_MEMBER_FN(__cdecl, retType, name, ##__VA_ARGS__)
#define DECL_HOOK_CDECL(retType, name, ...) _DECL_HOOK_FN(__cdecl, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_CDECL(retType, name, ...) _DECL_STATIC_FN(__cdecl, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_HOOK_CDECL(retType, name, ...) _DECL_STATIC_HOOK_FN(__cdecl, retType, name, ##__VA_ARGS__)
#define IMPL_HOOK_CDECL(spt_class, retType, name, ...) _IMPL_HOOK_FN(__cdecl, spt_class, retType, name, ##__VA_ARGS__)

// fastcall

#define DECL_MEMBER_FASTCALL(retType, name, ...) _DECL_MEMBER_FN(__fastcall, retType, name, ##__VA_ARGS__)
#define DECL_HOOK_FASTCALL(retType, name, ...) _DECL_HOOK_FN(__fastcall, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_FASTCALL(retType, name, ...) _DECL_STATIC_FN(__fastcall, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_HOOK_FASTCALL(retType, name, ...) _DECL_STATIC_HOOK_FN(__fastcall, retType, name, ##__VA_ARGS__)
#define IMPL_HOOK_FASTCALL(spt_class, retType, name, ...) \
	_IMPL_HOOK_FN(__fastcall, spt_class, retType, name, ##__VA_ARGS__)

// stdcall

#define DECL_MEMBER_STDCALL(retType, name, ...) _DECL_MEMBER_FN(__stdcall, retType, name, ##__VA_ARGS__)
#define DECL_HOOK_STDCALL(retType, name, ...) _DECL_HOOK_FN(__stdcall, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_STDCALL(retType, name, ...) _DECL_STATIC_FN(__stdcall, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_HOOK_STDCALL(retType, name, ...) _DECL_STATIC_HOOK_FN(__stdcall, retType, name, ##__VA_ARGS__)
#define IMPL_HOOK_STDCALL(spt_class, retType, name, ...) \
	_IMPL_HOOK_FN(__stdcall, spt_class, retType, name, ##__VA_ARGS__)

// naked
// Note that the return type and params don't actually matter for naked functions, but if this is
// the start of the function, it's good practice to include them.

#define DECL_MEMBER_NAKED(retType, name, ...) _DECL_MEMBER_FN(, retType, name, ##__VA_ARGS__)
#define DECL_HOOK_NAKED(retType, name, ...) _DECL_HOOK_FN(, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_NAKED(retType, name, ...) _DECL_STATIC_FN(, retType, name, ##__VA_ARGS__)
#define DECL_STATIC_HOOK_NAKED(retType, name, ...) _DECL_STATIC_HOOK_FN(, retType, name, ##__VA_ARGS__)
#define IMPL_HOOK_NAKED(spt_class, retType, name, ...) \
	_IMPL_HOOK_FN(, spt_class, __declspec(naked) retType, name, ##__VA_ARGS__)

// thiscall convention - msvc doesn't allow a static function to be thiscall, we make the ORIG function __thiscall
// & the static __fastcall with a hidden edx param (the callee is allowed to clobber edx)

#define DECL_MEMBER_THISCALL(retType, name, thisType, ...) \
	_DECL_MEMBER_FN(__thiscall, retType, name, thisType thisptr, ##__VA_ARGS__)

#define DECL_HOOK_THISCALL(retType, name, thisType, ...) \
	DECL_MEMBER_THISCALL(retType, name, thisType, ##__VA_ARGS__); \
	static retType __fastcall HOOKED_##name(thisType thisptr, int _edx, ##__VA_ARGS__)

#define DECL_STATIC_THISCALL(retType, name, thisType, ...) \
	_DECL_STATIC_FN(__thiscall, retType, name, thisType thisptr, ##__VA_ARGS__)

#define DECL_STATIC_HOOK_THISCALL(retType, name, thisType, ...) \
	DECL_STATIC_THISCALL(retType, name, thisType, ##__VA_ARGS__); \
	static retType __fastcall HOOKED_##name(thisType thisptr, int _edx, ##__VA_ARGS__)

#define IMPL_HOOK_THISCALL(spt_class, retType, name, thisType, ...) \
	_IMPL_HOOK_FN(__fastcall, spt_class, retType, name, thisType thisptr, int _edx, ##__VA_ARGS__)

// misc

#define ADD_RAW_HOOK(moduleName, name) \
	AddRawHook(#moduleName, reinterpret_cast<void**>(&ORIG_##name##), reinterpret_cast<void*>(HOOKED_##name##));
#define HOOK_FUNCTION(moduleName, name) \
	AddPatternHook(patterns::##name##, \
	               #moduleName, \
	               #name, \
	               reinterpret_cast<void**>(&ORIG_##name##), \
	               reinterpret_cast<void*>(HOOKED_##name##));

#define FIND_PATTERN(moduleName, name) \
	AddPatternHook(patterns::##name##, #moduleName, #name, reinterpret_cast<void**>(&ORIG_##name##), nullptr);
#define FIND_PATTERN_ALL(moduleName, name) \
	AddMatchAllPattern(patterns::##name##, #moduleName, #name, &MATCHES_##name##);

// direct byte replacements is only needed in very niche applications and quite dangerous,
// so all of this should stay as macros and a pain in the arse to use

#define DECL_BYTE_REPLACE(name, size, ...) \
	uintptr_t PTR_##name = NULL; \
	byte ORIG_BYTES_##name[size] = {}; \
	byte NEW_BYTES_##name[size] = {##__VA_ARGS__};
#define INIT_BYTE_REPLACE(name, ptr) \
	PTR_##name = ptr; \
	if (ptr != NULL) \
	memcpy((void*)ORIG_BYTES_##name, (void*)(ptr), sizeof(ORIG_BYTES_##name))
#define DO_BYTE_REPLACE(name) \
	if (PTR_##name != NULL) \
		MemUtils::ReplaceBytes((void*)PTR_##name, sizeof(ORIG_BYTES_##name), NEW_BYTES_##name);
#define UNDO_BYTE_REPLACE(name) \
	if (PTR_##name != NULL) \
		MemUtils::ReplaceBytes((void*)PTR_##name, sizeof(ORIG_BYTES_##name), ORIG_BYTES_##name);
#define DESTROY_BYTE_REPLACE(name) \
	if (PTR_##name != NULL) \
	{ \
		UNDO_BYTE_REPLACE(name); \
		PTR_##name = NULL; \
		memset((void*)ORIG_BYTES_##name, 0x00, sizeof(ORIG_BYTES_##name)); \
	}

#define _MAKE_WSTR(name) L#name
#define GET_MODULE(name) \
	void* name##Handle; \
	void* name##Base; \
	size_t name##Size = 0; \
	MemUtils::GetModuleInfo(_MAKE_WSTR(##name.dll), &##name##Handle, &##name##Base, &##name##Size);

#define InitCommand(command) InitConcommandBase(command##_command)

#else

#define _DECL_FN(qualifiers, call_conv, retType, name, ...) \
	using _##name = retType(call_conv*)(__VA_OPT__(__VA_ARGS__)); \
	qualifiers _##name ORIG_##name = nullptr

#define _DECL_MEMBER_FN(call_conv, retType, name, ...) _DECL_FN(, call_conv, retType, name __VA_OPT__(, ) __VA_ARGS__)

#define _DECL_HOOK_FN(call_conv, retType, name, ...) \
	_DECL_MEMBER_FN(call_conv, retType, name __VA_OPT__(, ) __VA_ARGS__); \
	static retType call_conv HOOKED_##name(__VA_OPT__(__VA_ARGS__))

#define _DECL_STATIC_FN(call_conv, retType, name, ...) \
	_DECL_FN(inline static, call_conv, retType, name __VA_OPT__(, ) __VA_ARGS__)

#define _DECL_STATIC_HOOK_FN(call_conv, retType, name, ...) \
	_DECL_STATIC_FN(call_conv, retType, name __VA_OPT__(, ) __VA_ARGS__); \
	static retType call_conv HOOKED_##name(__VA_OPT__(__VA_ARGS__))

#define _IMPL_HOOK_FN(call_conv, spt_class, retType, name, ...) \
	retType call_conv spt_class::HOOKED_##name(__VA_OPT__(__VA_ARGS__))

// cdecl

#define DECL_MEMBER_CDECL(retType, name, ...) _DECL_MEMBER_FN(__cdecl, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_HOOK_CDECL(retType, name, ...) _DECL_HOOK_FN(__cdecl, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_CDECL(retType, name, ...) _DECL_STATIC_FN(__cdecl, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_HOOK_CDECL(retType, name, ...) \
	_DECL_STATIC_HOOK_FN(__cdecl, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define IMPL_HOOK_CDECL(spt_class, retType, name, ...) \
	_IMPL_HOOK_FN(__cdecl, spt_class, retType, name __VA_OPT__(, ) __VA_ARGS__)

// fastcall

#define DECL_MEMBER_FASTCALL(retType, name, ...) _DECL_MEMBER_FN(__fastcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_HOOK_FASTCALL(retType, name, ...) _DECL_HOOK_FN(__fastcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_FASTCALL(retType, name, ...) _DECL_STATIC_FN(__fastcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_HOOK_FASTCALL(retType, name, ...) \
	_DECL_STATIC_HOOK_FN(__fastcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define IMPL_HOOK_FASTCALL(spt_class, retType, name, ...) \
	_IMPL_HOOK_FN(__fastcall, spt_class, retType, name __VA_OPT__(, ) __VA_ARGS__)

// stdcall

#define DECL_MEMBER_STDCALL(retType, name, ...) _DECL_MEMBER_FN(__stdcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_HOOK_STDCALL(retType, name, ...) _DECL_HOOK_FN(__stdcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_STDCALL(retType, name, ...) _DECL_STATIC_FN(__stdcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_HOOK_STDCALL(retType, name, ...) \
	_DECL_STATIC_HOOK_FN(__stdcall, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define IMPL_HOOK_STDCALL(spt_class, retType, name, ...) \
	_IMPL_HOOK_FN(__stdcall, spt_class, retType, name __VA_OPT__(, ) __VA_ARGS__)

// naked
// Note that the return type and params don't actually matter for naked functions, but if this is
// the start of the function, it's good practice to include them.

#define DECL_MEMBER_NAKED(retType, name, ...) _DECL_MEMBER_FN(, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_HOOK_NAKED(retType, name, ...) _DECL_HOOK_FN(, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_NAKED(retType, name, ...) _DECL_STATIC_FN(, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define DECL_STATIC_HOOK_NAKED(retType, name, ...) _DECL_STATIC_HOOK_FN(, retType, name __VA_OPT__(, ) __VA_ARGS__)
#define IMPL_HOOK_NAKED(spt_class, retType, name, ...) \
	_IMPL_HOOK_FN(, spt_class, __declspec(naked) retType, name __VA_OPT__(, ) __VA_ARGS__)

// thiscall convention - msvc doesn't allow a static function to be thiscall, we make the ORIG function __thiscall
// & the static __fastcall with a hidden edx param (the callee is allowed to clobber edx)

#define DECL_MEMBER_THISCALL(retType, name, thisType, ...) \
	_DECL_MEMBER_FN(__thiscall, retType, name, thisType thisptr __VA_OPT__(, ) __VA_ARGS__)

#define DECL_HOOK_THISCALL(retType, name, thisType, ...) \
	DECL_MEMBER_THISCALL(retType, name, thisType __VA_OPT__(, ) __VA_ARGS__); \
	static retType __fastcall HOOKED_##name(thisType thisptr, int _edx __VA_OPT__(, ) __VA_ARGS__)

#define DECL_STATIC_THISCALL(retType, name, thisType, ...) \
	_DECL_STATIC_FN(__thiscall, retType, name, thisType thisptr __VA_OPT__(, ) __VA_ARGS__)

#define DECL_STATIC_HOOK_THISCALL(retType, name, thisType, ...) \
	DECL_STATIC_THISCALL(retType, name, thisType __VA_OPT__(, ) __VA_ARGS__); \
	static retType __fastcall HOOKED_##name(thisType thisptr, int _edx __VA_OPT__(, ) __VA_ARGS__)

#define IMPL_HOOK_THISCALL(spt_class, retType, name, thisType, ...) \
	_IMPL_HOOK_FN(__fastcall, spt_class, retType, name, thisType thisptr, int _edx __VA_OPT__(, ) __VA_ARGS__)

// misc

#define ADD_RAW_HOOK(moduleName, name) \
	AddRawHook(#moduleName, reinterpret_cast<void**>(&ORIG_##name), reinterpret_cast<void*>(HOOKED_##name));
#define HOOK_FUNCTION(moduleName, name) \
	AddPatternHook(patterns::name, \
	               #moduleName, \
	               #name, \
	               reinterpret_cast<void**>(&ORIG_##name), \
	               reinterpret_cast<void*>(HOOKED_##name));

#define FIND_PATTERN(moduleName, name) \
	AddPatternHook(patterns::name, #moduleName, #name, reinterpret_cast<void**>(&ORIG_##name), nullptr);
#define FIND_PATTERN_ALL(moduleName, name) AddMatchAllPattern(patterns::name, #moduleName, #name, &MATCHES_##name);

// direct byte replacements is only needed in very niche applications and quite dangerous,
// so all of this should stay as macros and a pain in the arse to use

#define DECL_BYTE_REPLACE(name, size, ...) \
	uintptr_t PTR_##name = NULL; \
	byte ORIG_BYTES_##name[size] = {}; \
	byte NEW_BYTES_##name[size] = {__VA_OPT__(__VA_ARGS__)};
#define INIT_BYTE_REPLACE(name, ptr) \
	PTR_##name = ptr; \
	if (ptr != NULL) \
	memcpy((void*)ORIG_BYTES_##name, (void*)(ptr), sizeof(ORIG_BYTES_##name))
#define DO_BYTE_REPLACE(name) \
	if (PTR_##name != NULL) \
		MemUtils::ReplaceBytes((void*)PTR_##name, sizeof(ORIG_BYTES_##name), NEW_BYTES_##name);
#define UNDO_BYTE_REPLACE(name) \
	if (PTR_##name != NULL) \
		MemUtils::ReplaceBytes((void*)PTR_##name, sizeof(ORIG_BYTES_##name), ORIG_BYTES_##name);
#define DESTROY_BYTE_REPLACE(name) \
	if (PTR_##name != NULL) \
	{ \
		UNDO_BYTE_REPLACE(name); \
		PTR_##name = NULL; \
		memset((void*)ORIG_BYTES_##name, 0x00, sizeof(ORIG_BYTES_##name)); \
	}

#define _MAKE_WSTR2(x) L##x
#define _MAKE_WSTR(x) _MAKE_WSTR2(#x)
#define GET_MODULE(name) \
	void* name##Handle; \
	void* name##Base; \
	size_t name##Size = 0; \
	MemUtils::GetModuleInfo(_MAKE_WSTR(name) L".dll", &name##Handle, &name##Base, &name##Size)

#define InitCommand(command) InitConcommandBase(command##_command)

#endif // defined(_MSC_VER) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)

struct VFTableHook
{
	VFTableHook(void** vftable, int index, void* functionToHook, void** origPtr);

	void** vftable;
	int index;
	void* functionToHook;
	void** origPtr;
};

struct PatternHook
{
	PatternHook(patterns::PatternWrapper* patternArr,
	            size_t size,
	            const char* patternName,
	            void** origPtr,
	            void* functionHook)
	{
		this->patternArr = patternArr;
		this->size = size;
		this->patternName = patternName;
		this->origPtr = origPtr;
		this->functionHook = functionHook;
	}

	patterns::PatternWrapper* patternArr;
	size_t size;
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct MatchAllPattern
{
	MatchAllPattern(patterns::PatternWrapper* patternArr,
	                size_t size,
	                const char* patternName,
	                std::vector<patterns::MatchedPattern>* foundVec)
	{
		this->patternArr = patternArr;
		this->size = size;
		this->patternName = patternName;
		this->foundVec = foundVec;
	}

	patterns::PatternWrapper* patternArr;
	size_t size;
	const char* patternName;
	std::vector<patterns::MatchedPattern>* foundVec;
};

struct OffsetHook
{
	int32_t offset;
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct RawHook
{
	const char* patternName;
	void** origPtr;
	void* functionHook;
};

struct ModuleHookData
{
	std::vector<PatternHook> patternHooks;
	std::vector<MatchAllPattern> matchAllPatterns;
	std::vector<VFTableHook> vftableHooks;
	std::vector<OffsetHook> offsetHooks;

	std::vector<std::pair<void**, void*>> funcPairs;
	std::vector<void**> hookedFunctions;
	std::vector<VFTableHook> existingVTableHooks;
	void InitModule(const std::wstring& moduleName);
	void HookModule(const std::wstring& moduleName);
	void UnhookModule(const std::wstring& moduleName);
};

class Feature
{
public:
	virtual ~Feature(){};
	virtual bool ShouldLoadFeature()
	{
		return true;
	};
	virtual void InitHooks(){};
	virtual void PreHook(){};
	virtual void LoadFeature(){};
	virtual void UnloadFeature(){};
	virtual Feature* CreateNewInstance() = 0;
	virtual void Move(Feature* instance) = 0;

	static void ReloadFeatures();
	static void LoadFeatures();
	static void UnloadFeatures();

	template<size_t PatternLength>
	static void AddPatternHook(const std::array<patterns::PatternWrapper, PatternLength>& patterns,
	                           std::string moduleName,
	                           const char* patternName,
	                           void** origPtr = nullptr,
	                           void* functionHook = nullptr);
	template<size_t PatternLength>
	static void AddMatchAllPattern(const std::array<patterns::PatternWrapper, PatternLength>& patterns,
	                               std::string moduleName,
	                               const char* patternName,
	                               std::vector<patterns::MatchedPattern>* foundVec);
	static void AddRawHook(std::string moduleName, void** origPtr, void* functionHook);
	static void AddPatternHook(PatternHook hook, std::string moduleEnum);
	static void AddMatchAllPattern(MatchAllPattern hook, std::string moduleName);
	static void AddVFTableHook(VFTableHook hook, std::string moduleEnum);
	static void AddOffsetHook(std::string moduleName,
	                          int offset,
	                          const char* patternName,
	                          void** origPtr = nullptr,
	                          void* functionHook = nullptr);
	static int GetPatternIndex(void** origPtr);

	Feature();

protected:
	void InitConcommandBase(ConCommandBase& convar);
	bool AddHudCallback(const char* key, std::function<void(std::string)> func, ConVar& cvar);

	bool moduleLoaded;
	bool startedLoading;

private:
	static void InitModules();
	static void Hook();
	static void Unhook();
};

template<typename T>
class FeatureWrapper : public Feature
{
public:
	virtual Feature* CreateNewInstance()
	{
		return new T();
	}

	virtual void Move(Feature* instance)
	{
		*((T*)this) = std::move(*(T*)instance);
	}
};

template<size_t PatternLength>
inline void Feature::AddPatternHook(const std::array<patterns::PatternWrapper, PatternLength>& p,
                                    std::string moduleEnum,
                                    const char* patternName,
                                    void** origPtr,
                                    void* functionHook)
{
	AddPatternHook(PatternHook(const_cast<patterns::PatternWrapper*>(p.data()),
	                           PatternLength,
	                           patternName,
	                           origPtr,
	                           functionHook),
	               moduleEnum);
}

template<size_t PatternLength>
inline void Feature::AddMatchAllPattern(const std::array<patterns::PatternWrapper, PatternLength>& patterns,
                                        std::string moduleName,
                                        const char* patternName,
                                        std::vector<patterns::MatchedPattern>* foundVec)
{
	AddMatchAllPattern(MatchAllPattern(const_cast<patterns::PatternWrapper*>(patterns.data()),
	                                   PatternLength,
	                                   patternName,
	                                   foundVec),
	                   moduleName);
}
