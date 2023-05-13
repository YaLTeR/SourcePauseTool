#pragma once
#include "interface.h"
#include <array>
#include <vector>
#include <functional>
#include <unordered_map>
#include "SPTLib\patterns.hpp"
#include "SPTLib\memutils.hpp"
#include "convar.hpp"

// cdecl convention

#define DECL_MEMBER_CDECL(retType, name, ...) \
	using _##name = retType(__cdecl*)(__VA_ARGS__); \
	_##name ORIG_##name = nullptr;

#define DECL_HOOK_CDECL(retType, name, ...) \
	DECL_MEMBER_CDECL(retType, name, ##__VA_ARGS__) \
	static retType __cdecl HOOKED_##name(__VA_ARGS__)

#define IMPL_HOOK_CDECL(owner, retType, name, ...) retType __cdecl owner::HOOKED_##name(__VA_ARGS__)

// thiscall convention - msvc doesn't allow a static function to be thiscall, we make the ORIG function __thiscall
// & the static __fastcall with a hidden edx param (the callee is allowed to clobber edx)

#define DECL_MEMBER_THISCALL(retType, name, thisType, ...) \
	using _##name = retType(__thiscall*)(thisType thisptr, ##__VA_ARGS__); \
	_##name ORIG_##name = nullptr;

#define DECL_HOOK_THISCALL(retType, name, thisType, ...) \
	DECL_MEMBER_THISCALL(retType, name, thisType, ##__VA_ARGS__) \
	static retType __fastcall HOOKED_##name(thisType thisptr, int _edx, ##__VA_ARGS__)

#define IMPL_HOOK_THISCALL(owner, retType, name, thisType, ...) \
	retType __fastcall owner::HOOKED_##name(thisType thisptr, int _edx, ##__VA_ARGS__)

// fastcall convention

#define DECL_MEMBER_FASTCALL(retType, name, ...) \
	using _##name = retType(__fastcall*)(__VA_ARGS__); \
	_##name ORIG_##name = nullptr;

#define DECL_HOOK_FASTCALL(retType, name, ...) \
	DECL_MEMBER_FASTCALL(retType, name, ##__VA_ARGS__) \
	static retType __fastcall HOOKED_##name(__VA_ARGS__)

#define IMPL_HOOK_FASTCALL(owner, retType, name, ...) retType __fastcall owner::HOOKED_##name(__VA_ARGS__)

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
