#include "stdafx.h"

#include "ClientDLL.hpp"

#include <SPTLib\hooks.hpp>
#include <SPTLib\memutils.hpp>

#include "..\..\sptlib-wrapper.hpp"
#include "..\..\strafestuff.hpp"
#include "..\cvars.hpp"
#include "..\modules.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\patterns.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "..\scripts\tests\test.hpp"
#include "..\..\aim.hpp"
#include "bspflags.h"
#include "..\..\patterns_new.cpp"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using std::size_t;
using std::uintptr_t;

#define TAG "[client dll] "

void __cdecl ClientDLL::HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_DoImageSpaceMotionBlur_Func(view, x, y, w, h);
}

bool __fastcall ClientDLL::HOOKED_CheckJumpButton(void* thisptr, int edx)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_CheckJumpButton_Func(thisptr, edx);
}

void __stdcall ClientDLL::HOOKED_HudUpdate(bool bActive)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_HudUpdate_Func(bActive);
}

int __fastcall ClientDLL::HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_GetButtonBits_Func(thisptr, edx, bResetState);
}

void __fastcall ClientDLL::HOOKED_AdjustAngles(void* thisptr, int edx, float frametime)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_AdjustAngles_Func(thisptr, edx, frametime);
}

void __fastcall ClientDLL::HOOKED_CreateMove(void* thisptr,
                                             int edx,
                                             int sequence_number,
                                             float input_sample_frametime,
                                             bool active)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_CreateMove_Func(thisptr, edx, sequence_number, input_sample_frametime, active);
}

void __fastcall ClientDLL::HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_CViewRender__OnRenderStart_Func(thisptr, edx);
}

void ClientDLL::HOOKED_CViewRender__RenderView(void* thisptr,
                                               int edx,
                                               void* cameraView,
                                               int nClearFlags,
                                               int whatToDraw)
{
	TRACE_ENTER();
	clientDLL.HOOKED_CViewRender__RenderView_Func(thisptr, edx, cameraView, nClearFlags, whatToDraw);
}

void ClientDLL::HOOKED_CViewRender__Render(void* thisptr, int edx, void* rect)
{
	TRACE_ENTER();
	clientDLL.HOOKED_CViewRender__Render_Func(thisptr, edx, rect);
}

ConVar y_spt_disable_fade("y_spt_disable_fade", "0", FCVAR_ARCHIVE, "Disables all fades.");

void __fastcall ClientDLL::HOOKED_CViewEffects__Fade(void* thisptr, int edx, void* data)
{
	if (!y_spt_disable_fade.GetBool())
		clientDLL.ORIG_CViewEffects__Fade(thisptr, edx, data);
}

ConVar y_spt_disable_shake("y_spt_disable_shake", "0", FCVAR_ARCHIVE, "Disables all shakes.");

void __fastcall ClientDLL::HOOKED_CViewEffects__Shake(void* thisptr, int edx, void* data)
{
	if (!y_spt_disable_shake.GetBool())
		clientDLL.ORIG_CViewEffects__Shake(thisptr, edx, data);
}

#ifndef OE
void DisablePickupWeaponSound(IConVar* var, const char* pOldValue, float fOldValue)
#else
void DisablePickupWeaponSound(ConVar* var, char const* pOldString)
#endif
{
	if (clientDLL.ORIG_PickupWeaponPTR == nullptr)
	{
		ConWarning(1, "command has no effect!\n");
		return;
	}

	// the game queues the sound through a string, so let's just replace it
	char* oldText = (char*)(clientDLL.ORIG_PickupWeaponPTR);
	if (((ConVar*)var)->GetBool())
		strcpy(oldText, "PLAY AG");
	else
		strcpy(oldText, "Player.PickupWeapon");
}


ConVar y_spt_disable_weapon_pickup_sound("y_spt_disable_weapon_pickup_sound",
	                  "0",
	                  FCVAR_ARCHIVE,
	                  "Disables weapon pickup sounds.", DisablePickupWeaponSound);

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::client::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg(TAG "Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::client::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning(TAG "Could not find " #future_name ".\n"); \
		} \
	}

#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg(TAG "Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::client::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning(TAG "Could not find " #future_name ".\n"); \
		} \
	}

using namespace PatternsExt;

void ClientDLL::Hook(const std::wstring& moduleName,
                     void* moduleHandle,
                     void* moduleBase,
                     size_t moduleLength,
                     bool needToIntercept)
{
	auto startTime = std::chrono::high_resolution_clock::now();

	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	uintptr_t ORIG_MiddleOfCAM_Think, ORIG_CHLClient__CanRecordDemo, ORIG_CHudDamageIndicator__GetDamagePosition;

	patternContainer.Init(moduleName);

	const PatternScanner mScanner(moduleBase, moduleLength);

	DEF_FUTURE(HudUpdate);
	DEF_FUTURE(GetButtonBits);
	DEF_FUTURE(AdjustAngles);
	DEF_FUTURE(CreateMove);
	DEF_FUTURE(CViewRender__OnRenderStart);
	DEF_FUTURE(MiddleOfCAM_Think);
	DEF_FUTURE(GetGroundEntity);
	DEF_FUTURE(CalcAbsoluteVelocity);
	DEF_FUTURE(DoImageSpaceMotionBlur);
	DEF_FUTURE(CheckJumpButton);
	DEF_FUTURE(CHLClient__CanRecordDemo);
	DEF_FUTURE(CViewRender__RenderView);
	DEF_FUTURE(CViewRender__Render);
	DEF_FUTURE(UTIL_TraceRay);
	DEF_FUTURE(CGameMovement__CanUnDuckJump);
	DEF_FUTURE(CViewEffects__Fade);
	DEF_FUTURE(CViewEffects__Shake);
	DEF_FUTURE(CHudDamageIndicator__GetDamagePosition);
	DEF_FUTURE(MiddleOfCViewRenderRender);
	DEF_FUTURE(ResetToneMapping);
	GET_HOOKEDFUTURE(HudUpdate);
	GET_HOOKEDFUTURE(GetButtonBits);
	GET_HOOKEDFUTURE(AdjustAngles);
	GET_HOOKEDFUTURE(CreateMove);
	GET_HOOKEDFUTURE(CViewRender__OnRenderStart);
	GET_FUTURE(MiddleOfCAM_Think);
	GET_FUTURE(GetGroundEntity);
	GET_FUTURE(CalcAbsoluteVelocity);
	GET_HOOKEDFUTURE(DoImageSpaceMotionBlur);
	GET_HOOKEDFUTURE(CheckJumpButton);
	GET_FUTURE(CHLClient__CanRecordDemo);
	GET_HOOKEDFUTURE(CViewRender__RenderView);
	GET_HOOKEDFUTURE(CViewRender__Render);
	GET_FUTURE(UTIL_TraceRay);
	GET_FUTURE(CGameMovement__CanUnDuckJump);
	GET_HOOKEDFUTURE(CViewEffects__Fade);
	GET_HOOKEDFUTURE(CViewEffects__Shake);
	GET_FUTURE(CHudDamageIndicator__GetDamagePosition);
	GET_HOOKEDFUTURE(ResetToneMapping);
	DEF_FUTURE(PickupWeaponPTR);
	GET_FUTURE(PickupWeaponPTR);
	GET_FUTURE(MiddleOfCViewRenderRender);

	if (ORIG_MiddleOfCViewRenderRender)
	{
		unsigned char* bytes = (reinterpret_cast<unsigned char*>(ORIG_MiddleOfCViewRenderRender));
		int basePtr = *(int*)(bytes + 2);
		uint offset = 0x0;
		for (int i = 0; i < 20; i++)
		{
			if ((bytes[i] == 0x8B || bytes[i] == 0xFF) && bytes[i + 1] == 0x90)
				offset = (uint)(bytes[i + 2]);
		}

		pVGUI_Paint = *(int*)(**(int**)(basePtr) + offset);
		DevMsg("Engine VGUI_Paint found at %p\n", pVGUI_Paint);
	}

	if (ORIG_PickupWeaponPTR != nullptr)
	{
		DWORD dwOldProtect;
		VirtualProtect(ORIG_PickupWeaponPTR, 19, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	}

	if (DoesGameLookLikeHDTF())
	{
		DEF_FUTURE(HDTF_MiddleOfViewBobFuncStart);
		GET_HOOKEDFUTURE(HDTF_MiddleOfViewBobFuncStart);
		if (ORIG_HDTF_MiddleOfViewBobFuncStart != nullptr)
			ORIG_HDTF_MiddleOfViewBobFuncEnd = (uint)ORIG_HDTF_MiddleOfViewBobFuncStart + 0x34;

		DEF_FUTURE(HDTF_MiddleOfViewRollFunc);
		GET_HOOKEDFUTURE(HDTF_MiddleOfViewRollFunc);
		if (ORIG_HDTF_MiddleOfViewRollFunc != nullptr)
		{
			byte* offset = (byte*)((uint)ORIG_HDTF_MiddleOfViewRollFunc + 0x1);
			ORIG_HDTF_MiddleOfViewRollFunc_JumpTo = (uint)((uint)ORIG_HDTF_MiddleOfViewRollFunc + 0x2 + *offset);
		}

	}

	if (DoesGameLookLikeHLS())
	{
		sizeofCUserCmd = 84 - sizeof(CUtlVector<int>);
	}
	else
	{
		sizeofCUserCmd = 84;
	}

	if (ORIG_DoImageSpaceMotionBlur)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_DoImageSpaceMotionBlur);

		switch (ptnNumber)
		{
		case 0:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 132);
			break;

		case 1:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 153);
			break;

		case 2:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 129);
			break;

		case 3:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 171);
			break;

		case 4:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 177);
			break;

		case 5:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 128);
			break;
		}

		DevMsg(TAG "pgpGlobals is %p.\n", pgpGlobals);
	}
	else
	{
		GENERIC_BACKTRACE_NOTE(DoImageSpaceMotionBlur)
		uintptr_t tmp = FindStringAddress(mScanner, "dev/motion_blur");
		tmp = FindVarReference(mScanner, tmp, "68 ");
		tmp = BackTraceToFuncStart(mScanner, tmp, 0x1000, 4, false, 0x10000);

		if (tmp != 0x0)
		{
			DevMsg(TAG "DoImageSpaceMotionBlur found at %p through function backtracing\n", tmp);
			ORIG_DoImageSpaceMotionBlur = (_DoImageSpaceMotionBlur)tmp;
			patternContainer.AddHook(HOOKED_DoImageSpaceMotionBlur, (PVOID*)&ORIG_DoImageSpaceMotionBlur);
		}
		else
			Warning("y_spt_motion_blur_fix has no effect.\n");
	}

	if (ORIG_DoImageSpaceMotionBlur && pgpGlobals == 0)
	{
		DevWarning(1, TAG "DoImageSpaceMotionBlur doesn't have a switch case for finding pgpGlobals, searching automatically instead...\n");
		uintptr_t tmp = (uintptr_t)ORIG_DoImageSpaceMotionBlur;
		PatternScanner scanner((void*)tmp, 0x200);
		Pattern p("F3 0F 10 ?? ?? ?? ?? ??", 4);
		p.onMatchEvaluate = _oMEArgs(&mScanner) { 
			*done = (mScanner.CheckWithin(*(uintptr_t*)*foundPtr)) && **(float**)(*foundPtr) == 180.0f; 
		};
		tmp = scanner.Scan(p);

		if (tmp != 0)
		{
			unsigned char* bytes = (unsigned char*)tmp;
			for (int i = 0; i < 0x100; i++)
			{
				if (bytes[-i] == 0xA1)
				{
					pgpGlobals = *(uintptr_t**)(bytes - i + 1);
					DevMsg(TAG "pgpGlobals is %p.\n", pgpGlobals);
					break;
				}
			}
		}
	}

	if (ORIG_CheckJumpButton)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_CheckJumpButton);

		switch (ptnNumber)
		{
		case 0:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;
		case 1:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 2:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 3:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 4:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 5:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 6:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 7:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 8:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 9:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 10:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
		}
	}
	else
	{
		Warning("y_spt_autojump has no effect in multiplayer.\n");
	}

	if (!ORIG_HudUpdate)
	{
		GENERIC_BACKTRACE_NOTE(HudUpdate);
		uintptr_t tmp = FindStringAddress(mScanner, "(time_float)");
		tmp = FindVarReference(mScanner, tmp, "68");
		tmp = BackTraceToFuncStart(mScanner, tmp, 0x300, 3, true);

		if (tmp != 0)
		{
			DevMsg(TAG "Found HudUpdate at %p through function backtracing\n", tmp);
			ORIG_HudUpdate = (_HudUpdate)tmp;
			patternContainer.AddHook(HOOKED_HudUpdate, (PVOID*)&ORIG_HudUpdate);
		}
		else
		{
			DevWarning(1, TAG "Method 1 for finding HudUpdate failed, trying VFTable jumping from LevelInitPreEntity instead...\n");
			tmp = FindStringAddress(mScanner, "cl_predict 1");
			tmp = FindVarReference(mScanner, tmp, "68");
			tmp = BackTraceToFuncStart(mScanner, tmp, 0x300, 3, true);
			if (tmp != 0)
			{
				DevMsg(TAG "Found LevelInitPreEntity at %p\n", tmp);
				Pattern p = GeneratePatternFromVar(tmp);
				p.onMatchEvaluate = _oMEArgs(&){
					if (mScanner.CheckWithin(*(uintptr_t*)(*foundPtr + 4))
						&& * foundPtr % 4 == 0)
					{
						// hudupdate should always be 6 vftable entires away...
						*foundPtr = *(uintptr_t*)(*foundPtr + 4 * 6);
						DevMsg(TAG "Found HudUpdate at %p through VFTable jumping\n", *foundPtr);
						ORIG_HudUpdate = (_HudUpdate)*foundPtr;
						patternContainer.AddHook(HOOKED_HudUpdate, (PVOID*)&ORIG_HudUpdate);
						*done = true;
					}
					else
						*done = false;
				};
				mScanner.Scan(p);
			}
			else
				Warning("_y_spt_afterframes has no effect.\n");
		}
	}

	if (!ORIG_GetButtonBits)
	{
		GENERIC_BACKTRACE_NOTE(GetButtonBits);
		PatternCollection p("81 ce 00 00 20 00", 0);
		p.AddPattern("0d 00 00 20 00", 0);
		uintptr_t tmp = mScanner.Scan(p);
		tmp = BackTraceToFuncStart(mScanner, tmp, 0x1000, 3, true);
		if (tmp != 0)
		{
			DevMsg(TAG "GetButtonBits found at %p through function backtracing\n", tmp);
			ORIG_GetButtonBits = (_GetButtonBits)tmp;
			patternContainer.AddHook(HOOKED_GetButtonBits, (PVOID*)&ORIG_GetButtonBits);
		}
		else
			Warning("+y_spt_duckspam has no effect.\n");
	}

	if (ORIG_CreateMove)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_CreateMove);

		switch (ptnNumber)
		{
		case 0:
			offM_pCommands = 180;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 1:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 2:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 3:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 4:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;
		}
	}

	if (!ORIG_AdjustAngles || !ORIG_CreateMove)
	{
		Warning("_y_spt_setangles has no effect.\n");
		Warning("_y_spt_setpitch and _y_spt_setyaw have no effect.\n");
		Warning("_y_spt_pitchspeed and _y_spt_yawspeed have no effect.\n");
	}

	if (ORIG_GetGroundEntity)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_GetGroundEntity);
		switch (ptnNumber)
		{
		case 0:
			offMaxspeed = 4136;
			offFlags = 736;
			offAbsVelocity = 248;
			offDucking = 3545;
			offDuckJumpTime = 3552;
			offServerPreviouslyPredictedOrigin = 3692;
			offServerAbsOrigin = 580;
			offServerSurfaceFriction = 3812;
			break;

		case 1:
			offMaxspeed = 4076;
			offFlags = 732;
			offAbsVelocity = 244;
			offDucking = 3489;
			offDuckJumpTime = 3496;
			offServerPreviouslyPredictedOrigin = 3628;
			offServerAbsOrigin = 580;
			offServerSurfaceFriction = 3752;
			break;

		case 2:
			offMaxspeed = 4312;
			offFlags = 844;
			offAbsVelocity = 300;
			offDucking = 3713;
			offDuckJumpTime = 3720;
			offServerPreviouslyPredictedOrigin = 3752;
			offServerAbsOrigin = 636;
			offServerSurfaceFriction = 3872;
			break;

		case 3:
			offMaxspeed = 4320;
			offFlags = 844;
			offAbsVelocity = 300;
			offDucking = 3721;
			offDuckJumpTime = 3728;
			offServerPreviouslyPredictedOrigin = 3752;
			offServerAbsOrigin = 636;
			offServerSurfaceFriction = 3872;
			break;

		default:
			Warning("GetGroundEntity did not contain matching if statement for pattern!\n");
			goto here;
			break;
		}
	}
	else
	{
		here:
		DevMsg("GetGroundEntity not found! Using datatable data to get entity offset!\n");

		uintptr_t string_DT_loc;
		// DT_BASEPLAYER
		string_DT_loc = FindDataTable(mScanner, "DT_BasePlayer");
		offMaxspeed = FindEntityOffsetThroughDT(moduleBase, moduleLength, string_DT_loc, "m_flMaxspeed", 0);
		offFlags = FindEntityOffsetThroughDT(moduleBase, moduleLength, string_DT_loc, "m_fFlags", 0);

		// DT_LOCALPLAYEREXCLUSIVE
		int dt_local_off = 0;
		offVecVelocity = 0;
		string_DT_loc = FindDataTable(mScanner, "DT_LocalPlayerExclusive");
		dt_local_off = FindEntityOffsetThroughDT(moduleBase, moduleLength, string_DT_loc, "m_Local", 0);
		offVecVelocity = FindEntityOffsetThroughDT(moduleBase, moduleLength, string_DT_loc, "m_vecVelocity[0]", 0);

		// DT_LOCAL
		string_DT_loc = FindDataTable(mScanner, "DT_Local");
		offDucking = FindEntityOffsetThroughDT(moduleBase, moduleLength, string_DT_loc, "m_bDucking", dt_local_off);
		offDuckJumpTime = FindEntityOffsetThroughDT(moduleBase, moduleLength, string_DT_loc, "m_flDuckJumpTime", dt_local_off);

		if (offVecVelocity != 0)
		{
			uintptr_t tmp = 0;
			char sig[256];
			char byte1[14]; pUtils.toHexArray(offVecVelocity, byte1);
			char byte2[14]; pUtils.toHexArray(offVecVelocity + 4, byte2);

			// some games use mov, some game use fld and fstp, luckily these take the same bytes
			// this *could* break but unlikely
			sprintf(sig, "?? ?? %s ?? ?? ?? ?? 00 00 ?? ?? %s ?? ?? ?? ?? 00 00", byte1, byte2);
			tmp = mScanner.Scan(Pattern(sig, 0));

			if (tmp)
			{
				offAbsVelocity = *(int*)(tmp + 8);
				DevMsg("client m_vecAbsVelocity entity offset found at 0x%X\n", offAbsVelocity);

				if (!ORIG_CalcAbsoluteVelocity)
				{
					auto newPtr = BackTraceToFuncStart(mScanner, tmp, 0x200, 3, false, 0x10000);
					if (newPtr != 0x0)
					{
						ORIG_CalcAbsoluteVelocity = (_CalcAbsoluteVelocity)(newPtr);
						DevMsg(
						    TAG "CalcAbsoluteVelocity found at %p through function backtracking\n",
						    ORIG_CalcAbsoluteVelocity);
					}
				}
			}
		}
	}

	if (ORIG_MiddleOfCAM_Think)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_MiddleOfCAM_Think);
		switch (ptnNumber)
		{
		case 0:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 29) + ORIG_MiddleOfCAM_Think + 33);
			break;

		case 1:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 30) + ORIG_MiddleOfCAM_Think + 34);
			break;

		case 2:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 21) + ORIG_MiddleOfCAM_Think + 25);
			break;

		case 3:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 23) + ORIG_MiddleOfCAM_Think + 27);
			break;
		}

		DevMsg(TAG "Found GetLocalPlayer at %p.\n", ORIG_GetLocalPlayer);
	}
	else
	{
		uintptr_t mCAM_stringPtr;
		mCAM_stringPtr = FindStringAddress(mScanner, "Pitch: %6.1f   Yaw: %6.1f %38s");
		mCAM_stringPtr = FindVarReference(mScanner, mCAM_stringPtr, "68");

		if (mCAM_stringPtr)
		{
			PatternScanner scanner((void*)(mCAM_stringPtr - 0x300), 0x320);
			PatternCollection p("E8 ?? ?? ?? FF", 1);
			p.AddPattern("E8 ?? ?? ?? 00", 1);

			p.onMatchEvaluate = _oMEArgs(&){
				unsigned char* bytes2 = (unsigned char*)(*foundPtr + 4 + *(int*)(*foundPtr));
				if (bytes2[0] == 0xa1 && bytes2[5] == 0xc3)
				{
					ORIG_GetLocalPlayer = (_GetLocalPlayer)(reinterpret_cast<uintptr_t>(bytes2));
					DevMsg(TAG "Found GetLocalPlayer at %p.\n", ORIG_GetLocalPlayer);
					*done = true;
				}
				*done = false;
			};

			scanner.ScanBackward(p, mCAM_stringPtr);
		}
	}


	if (ORIG_CHLClient__CanRecordDemo)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHLClient__CanRecordDemo + 1);
		ORIG_GetClientModeNormal = (_GetClientModeNormal)(offset + ORIG_CHLClient__CanRecordDemo + 5);
		DevMsg("[client.dll] Found GetClientModeNormal at %p\n", ORIG_GetClientModeNormal);
	}

	if (ORIG_CHudDamageIndicator__GetDamagePosition)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHudDamageIndicator__GetDamagePosition + 4);
		ORIG_MainViewOrigin = (_MainViewOrigin)(offset + ORIG_CHudDamageIndicator__GetDamagePosition + 8);
		DevMsg("[client.dll] Found MainViewOrigin at %p\n", ORIG_MainViewOrigin);
	}

	extern bool FoundEngineServer();
	if (ORIG_CreateMove && ORIG_GetGroundEntity && ORIG_CalcAbsoluteVelocity && ORIG_GetLocalPlayer
	    && ORIG_GetButtonBits && _sv_airaccelerate && _sv_accelerate && _sv_friction && _sv_maxspeed
	    && _sv_stopspeed && ORIG_AdjustAngles && FoundEngineServer())
	{
		tasAddressesWereFound = true;
	}
	else
	{
		Warning("The full game TAS solutions are not available.\n");
	}

	if (!ORIG_CViewRender__OnRenderStart)
	{
		Warning("_y_spt_force_90fov has no effect.\n");
	}

	if (ORIG_CViewRender__RenderView == nullptr || ORIG_CViewRender__Render == nullptr)
		Warning("Overlay cameras have no effect.\n");

	if (!ORIG_UTIL_TraceRay)
		Warning("tas_strafe_version 1 not available\n");

	if (!ORIG_CViewEffects__Fade || !ORIG_CViewEffects__Shake)
	{
		GENERIC_BACKTRACE_NOTE(CViewEffects Fade or Shake);
		uintptr_t tmp = FindStringAddress(mScanner, "%02d: dur(%8.2f) amp(%8.2f) freq(%8.2f)");
		tmp = FindVarReference(mScanner, tmp, "68");
		if (tmp != 0)
		{
			tmp = BackTraceToFuncStart(mScanner, tmp, 0x300, 3, true);
			tmp = FindVarReference(mScanner, tmp);
			if (tmp % 4 == 0 && mScanner.CheckWithin(*(uintptr_t*)tmp + 4))
			{
				if (!ORIG_CViewEffects__Fade)
				{
					ORIG_CViewEffects__Fade = *(_CViewEffects__Fade*)(tmp - 0xC);
					patternContainer.AddHook(HOOKED_CViewEffects__Fade,
					                         (PVOID*)&ORIG_CViewEffects__Fade);
					DevMsg(TAG "Found CViewEffects::Fade at %p through function backtracing\n",
					       ORIG_CViewEffects__Fade);
				}

				if (!ORIG_CViewEffects__Shake)
				{
					ORIG_CViewEffects__Shake = *(_CViewEffects__Shake*)(tmp - 0x10);
					patternContainer.AddHook(HOOKED_CViewEffects__Shake,
					                         (PVOID*)&ORIG_CViewEffects__Shake);
					DevMsg(TAG "Found CViewEffects::Shake at %p through function backtracing\n",
					       ORIG_CViewEffects__Shake);
				}
			}
		}
	}

	if (!ORIG_CViewEffects__Fade)
		Warning("y_spt_disable_fade 1 not available\n");

	if (!ORIG_CViewEffects__Shake)
		Warning("y_spt_disable_shake 1 not available\n");

	if (!ORIG_MainViewOrigin || !ORIG_UTIL_TraceRay)
		Warning("y_spt_hud_oob 1 has no effect\n");
	
	if (!ORIG_ResetToneMapping)
	{
		DevMsg("ResetToneMapping couldn't be found using existing patterns! Trying string reference instead...\n");
		uintptr_t ptr = FindVarReference(mScanner, FindStringAddress(mScanner, "(mapname)"), "68");
		if (ptr != 0)
		{
			PatternScanner scanner((void*)ptr, 0x200);
			PatternCollection p("D9 E8", 0);
			p.AddPattern("00 00 80 3F",0 );
			p.onMatchEvaluate = _oMEArgs(&mScanner) {
				unsigned char* bytes = (unsigned char*)*foundPtr;
				for (int i = 0; i < 50; i++)
				{
					if (bytes[i] == 0xE8 && mScanner.CheckWithin(READ_CALL(&bytes[i])))
					{
						*done = true;
						*foundPtr = READ_CALL(&bytes[i]);
						return;
					}
				}
				*foundPtr = 0;
			};
			ptr = scanner.Scan(p);
			
			if (ptr != 0)
			{
				DevMsg("ResetToneMapping found at %p through string reference\n", ptr);
				ORIG_ResetToneMapping = (_ResetToneMapping)ptr;
				patternContainer.AddHook(HOOKED_ResetToneMapping, (PVOID*)&ORIG_ResetToneMapping);
				goto rtm_eof;
			}
		}
	rtm_fail:
		Warning("y_spt_disable_tone_map_reset has no effect\n");
	rtm_eof:;
	}

	patternContainer.Hook();

	auto loadTime =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	DevMsg(TAG "Done hooking in %dms\n", loadTime);
}

void ClientDLL::Unhook()
{
	patternContainer.Unhook();
	Clear();
}

void ClientDLL::Clear()
{
	IHookableNameFilter::Clear();

	if (ORIG_PickupWeaponPTR != nullptr)
	{
		char* oldText = (char*)(ORIG_PickupWeaponPTR);
		strcpy(oldText, "Player.PickupWeapon");
	}

	ORIG_PickupWeaponPTR = nullptr;

	ORIG_HDTF_MiddleOfViewBobFuncStart = nullptr;
	ORIG_HDTF_MiddleOfViewBobFuncEnd = 0x0;

	ORIG_HDTF_MiddleOfViewRollFunc = nullptr;
	ORIG_HDTF_MiddleOfViewRollFunc_JumpTo = 0x0;

	ORIG_DoImageSpaceMotionBlur = nullptr;
	ORIG_CheckJumpButton = nullptr;
	ORIG_HudUpdate = nullptr;
	ORIG_GetButtonBits = nullptr;
	ORIG_AdjustAngles = nullptr;
	ORIG_CreateMove = nullptr;
	ORIG_GetLocalPlayer = nullptr;
	ORIG_GetGroundEntity = nullptr;
	ORIG_CalcAbsoluteVelocity = nullptr;
	ORIG_CViewRender__RenderView = nullptr;
	ORIG_CViewRender__Render = nullptr;
	ORIG_UTIL_TraceRay = nullptr;
	ORIG_MainViewOrigin = nullptr;
	ORIG_ResetToneMapping = nullptr;

	pgpGlobals = nullptr;
	off1M_nOldButtons = 0;
	off2M_nOldButtons = 0;
	offM_pCommands = 0;
	offForwardmove = 0;
	offSidemove = 0;
	offMaxspeed = 0;
	offFlags = 0;
	offAbsVelocity = 0;
	offDucking = 0;
	offDuckJumpTime = 0;
	offServerSurfaceFriction = 0;
	offServerPreviouslyPredictedOrigin = 0;
	offServerAbsOrigin = 0;
	pCmd = 0;

	tasAddressesWereFound = false;

	afterframesQueue.clear();
	afterframesPaused = false;

	duckspam = false;

	setPitch.set = false;
	setYaw.set = false;
	forceJump = false;
	cantJumpNextTime = false;
}

void ClientDLL::DelayAfterframesQueue(int delay)
{
	afterframesDelay = delay;
}

void ClientDLL::AddIntoAfterframesQueue(const afterframes_entry_t& entry)
{
	afterframesQueue.push_back(entry);
}

void ClientDLL::ResetAfterframesQueue()
{
	afterframesQueue.clear();
}

Strafe::MovementVars ClientDLL::GetMovementVars()
{
	TRACE_ENTER();
	auto vars = Strafe::MovementVars();

	if (!tasAddressesWereFound)
	{
		TRACE_EXIT();
		return Strafe::MovementVars();
	}

	auto player = ORIG_GetLocalPlayer();
	auto maxspeed = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offMaxspeed);

	auto pl = GetPlayerData();
	vars.OnGround = Strafe::GetPositionType(pl, pl.Ducking ? Strafe::HullType::DUCKED : Strafe::HullType::NORMAL)
	                == Strafe::PositionType::GROUND;
	bool ground; // for backwards compatibility with old bugs

	if (tas_strafe_version.GetInt() <= 1)
	{
		ground = IsGroundEntitySet();
	}
	else
	{
		ground = vars.OnGround;
	}

	if (tas_force_onground.GetBool())
	{
		ground = true;
		vars.OnGround = true;
	}

	vars.ReduceWishspeed = ground && GetFlagsDucking();
	vars.Accelerate = _sv_accelerate->GetFloat();

	if (tas_force_airaccelerate.GetString()[0] != '\0')
		vars.Airaccelerate = tas_force_airaccelerate.GetFloat();
	else
		vars.Airaccelerate = _sv_airaccelerate->GetFloat();

	vars.EntFriction = 1;
	vars.Frametime = 0.015f;
	vars.Friction = _sv_friction->GetFloat();
	vars.Maxspeed = (maxspeed > 0) ? std::min(maxspeed, _sv_maxspeed->GetFloat()) : _sv_maxspeed->GetFloat();
	vars.Stopspeed = _sv_stopspeed->GetFloat();

	if (tas_force_wishspeed_cap.GetString()[0] != '\0')
		vars.WishspeedCap = tas_force_wishspeed_cap.GetFloat();
	else
		vars.WishspeedCap = 30;

	extern IServerUnknown* GetServerPlayer();
	auto server_player = GetServerPlayer();

	auto previouslyPredictedOrigin =
	    reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(server_player) + offServerPreviouslyPredictedOrigin);
	auto absOrigin = reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(server_player) + offServerAbsOrigin);
	bool gameCodeMovedPlayer = (*previouslyPredictedOrigin != *absOrigin);

	vars.EntFriction =
	    *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(server_player) + offServerSurfaceFriction);

	if (gameCodeMovedPlayer)
	{
		if (tas_reset_surface_friction.GetBool())
		{
			vars.EntFriction = 1.0f;
		}

		if (pl.Velocity.z <= 140.f)
		{
			if (ground)
			{
				// TODO: This should check the real surface friction.
				vars.EntFriction = 1.0f;
			}
			else if (pl.Velocity.z > 0.f)
			{
				vars.EntFriction = 0.25f;
			}
		}
	}

	vars.EntGravity = 1.0f;
	vars.Maxvelocity = _sv_maxvelocity->GetFloat();
	vars.Gravity = _sv_gravity->GetFloat();
	vars.Stepsize = _sv_stepsize->GetFloat();
	vars.Bounce = _sv_bounce->GetFloat();

	TRACE_EXIT();

	return vars;
}

Strafe::PlayerData ClientDLL::GetPlayerData()
{
	if (!tasAddressesWereFound || !cinput_thisptr)
		return Strafe::PlayerData();

	Strafe::PlayerData data;
	const int IN_DUCK = 1 << 2;

	data.Ducking = GetFlagsDucking();
	data.DuckPressed = (ORIG_GetButtonBits(cinput_thisptr, 0, 0) & IN_DUCK);
	data.UnduckedOrigin =
	    *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + offServerAbsOrigin);
	data.Velocity = GetPlayerVelocity();
	data.Basevelocity = Vector();

	if (data.Ducking)
	{
		data.UnduckedOrigin.z -= 36;

		if (tas_strafe_use_tracing.GetBool() && Strafe::CanUnduck(data))
			data.Ducking = false;
	}

	return data;
}

Vector ClientDLL::GetPlayerVelocity()
{
	if (!ORIG_GetLocalPlayer || offAbsVelocity == 0x0)
		return Vector();
	auto player = ORIG_GetLocalPlayer();

	ORIG_CalcAbsoluteVelocity(player, 0);
	float* vel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offAbsVelocity);

	return Vector(vel[0], vel[1], vel[2]);
}

Vector ClientDLL::GetPlayerVecVelocity() 
{
	if (!ORIG_GetLocalPlayer)
		return Vector();

	auto player = ORIG_GetLocalPlayer();
	float* vel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offVecVelocity);

	return Vector(vel[0], vel[1], vel[2]);
}

Vector ClientDLL::GetPlayerEyePos()
{
	Vector rval = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + offServerAbsOrigin);
	constexpr float duckOffset = 28;
	constexpr float standingOffset = 64;

	if (GetFlagsDucking())
	{
		rval.z += duckOffset;
	}
	else
	{
		rval.z += standingOffset;
	}

	return rval;
}

Vector ClientDLL::GetCameraOrigin()
{
	if (!ORIG_MainViewOrigin)
		return Vector();
	return ORIG_MainViewOrigin();
}

int ClientDLL::GetPlayerFlags()
{
	if (!ORIG_GetLocalPlayer)
		return 0;
	return (*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ORIG_GetLocalPlayer()) + offFlags));
}

bool ClientDLL::GetFlagsDucking()
{
	if (!ORIG_GetLocalPlayer)
		return false;
	return (*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ORIG_GetLocalPlayer()) + offFlags)) & FL_DUCKING;
}

double ClientDLL::GetDuckJumpTime()
{
	if (!ORIG_GetLocalPlayer)
		return 0;

	auto player = ORIG_GetLocalPlayer();
	return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offDuckJumpTime);
}

bool ClientDLL::CanUnDuckJump(trace_t& ptr)
{
	if (!ORIG_CGameMovement__CanUnDuckJump)
	{
		Warning("Tried to run CanUnduckJump without the pattern!\n");
		return false;
	}

	return ORIG_CGameMovement__CanUnDuckJump(GetGamemovement(), 0, ptr);
}

void ClientDLL::OnFrame()
{
	FrameSignal();

	if (afterframesPaused)
	{
		return;
	}

	if (afterframesDelay-- > 0)
	{
		return;
	}

	for (auto it = afterframesQueue.begin(); it != afterframesQueue.end();)
	{
		it->framesLeft--;
		if (it->framesLeft <= 0)
		{
			EngineConCmd(it->command.c_str());
			it = afterframesQueue.erase(it);
		}
		else
			++it;
	}

	AfterFramesSignal();
}

void ClientDLL::ServerGameFrame() 
{
	if (pgpGlobals != 0)
		TickQueue.Update((*(CGlobalVarsBase**)pgpGlobals)->tickcount);
}

bool ClientDLL::IsGroundEntitySet()
{
	if (ORIG_GetGroundEntity == nullptr || ORIG_GetLocalPlayer == nullptr)
		return false;

	auto player = ORIG_GetLocalPlayer();
	return (ORIG_GetGroundEntity(player, 0) != NULL); // TODO: This should really be a proper check.
}

void __cdecl ClientDLL::HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h)
{
	uintptr_t origgpGlobals = NULL;

	/*
	Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
	so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
	function does not use anything apart from gpGlobals->realtime from gpGlobals,
	so we can do such a replace to make it use gpGlobals->curtime instead without
	breaking anything else.
	*/
	if (pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			origgpGlobals = *pgpGlobals;
			*pgpGlobals = *pgpGlobals + 12;
		}
	}

	ORIG_DoImageSpaceMotionBlur(view, x, y, w, h);

	if (pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			*pgpGlobals = origgpGlobals;
		}
	}
}

bool __fastcall ClientDLL::HOOKED_CheckJumpButton_Func(void* thisptr, int edx)
{
	/*
	Not sure if this gets called at all from the client dll, but
	I will just hook it in exactly the same way as the server one.
	*/
	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons = (int*)(*((uintptr_t*)thisptr + off1M_nOldButtons) + off2M_nOldButtons);
		origM_nOldButtons = *pM_nOldButtons;

		if (!cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
		else
		{
			// DevMsg( "Con jump prevented!\n" );
		}
	}

	cantJumpNextTime = false;

	bool rv = ORIG_CheckJumpButton(thisptr, edx); // This function can only change the jump bit.

	if (y_spt_autojump.GetBool())
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		// We jumped.
		if (_y_spt_autojump_ensure_legit.GetBool())
		{
			cantJumpNextTime = true; // Prevent consecutive jumps.
		}

		// DevMsg( "Jump!\n" );
	}

	DevMsg("Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false"));

	return rv;
}

void __stdcall ClientDLL::HOOKED_HudUpdate_Func(bool bActive)
{
	OnFrame();

	return ORIG_HudUpdate(bActive);
}

int __fastcall ClientDLL::HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState)
{
	int rv = ORIG_GetButtonBits(thisptr, edx, bResetState);

	if (bResetState == 1)
	{
		static bool duckPressed = false;

		if (duckspam)
		{
			if (duckPressed)
				duckPressed = false;
			else
			{
				duckPressed = true;
				rv |= (1 << 2); // IN_DUCK
			}
		}
		else
			duckPressed = false;

		if (forceJump)
		{
			forceJump = false;
			rv |= (1 << 1); // IN_JUMP
		}

		if (forceUnduck)
		{
			rv ^= (1 << 2); // IN_DUCK
			forceUnduck = false;
		}
	}

	return rv;
}

bool DoAngleChange(float& angle, float target)
{
	float normalizedDiff = utils::NormalizeDeg(target - angle);
	if (std::abs(normalizedDiff) > _y_spt_anglesetspeed.GetFloat())
	{
		angle += std::copysign(_y_spt_anglesetspeed.GetFloat(), normalizedDiff);
		return true;
	}
	else
	{
		angle = target;
		return false;
	}
}

void __fastcall ClientDLL::HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime)
{
	TRACE_ENTER();
	cinput_thisptr = thisptr;
	ORIG_AdjustAngles(thisptr, edx, frametime);

	if (!pCmd)
		return;
	float va[3];
	EngineGetViewAngles(va);
	bool yawChanged = false;

	// Use tas_aim stuff for tas_strafe_version >= 4
	if (tas_strafe_version.GetInt() >= 4)
	{
		aim::UpdateView(va[PITCH], va[YAW]);
	}

	double pitchSpeed = atof(_y_spt_pitchspeed.GetString()), yawSpeed = atof(_y_spt_yawspeed.GetString());

	if (pitchSpeed != 0.0f)
		va[PITCH] += pitchSpeed;
	if (setPitch.set)
	{
		setPitch.set = DoAngleChange(va[PITCH], setPitch.angle);
	}

	if (yawSpeed != 0.0f)
	{
		va[YAW] += yawSpeed;
	}
	if (setYaw.set)
	{
		yawChanged = true;
		setYaw.set = DoAngleChange(va[YAW], setYaw.angle);
	}

	if (tasAddressesWereFound && tas_strafe.GetBool())
	{
		auto player = ORIG_GetLocalPlayer();
		auto vars = GetMovementVars();
		auto pl = GetPlayerData();

		// Lgagst requires more prediction that is done here for correct operation.
		auto curState = Strafe::CurrentState();
		curState.LgagstMinSpeed = tas_strafe_lgagst_minspeed.GetFloat();
		curState.LgagstFullMaxspeed = tas_strafe_lgagst_fullmaxspeed.GetBool();

		bool jumped = false;
		const int IN_JUMP = (1 << 1);

		auto btns = Strafe::StrafeButtons();
		bool usingButtons = (sscanf(tas_strafe_buttons.GetString(),
		                            "%hhu %hhu %hhu %hhu",
		                            &btns.AirLeft,
		                            &btns.AirRight,
		                            &btns.GroundLeft,
		                            &btns.GroundRight)
		                     == 4);
		auto type = static_cast<Strafe::StrafeType>(tas_strafe_type.GetInt());
		auto dir = static_cast<Strafe::StrafeDir>(tas_strafe_dir.GetInt());

		Strafe::ProcessedFrame out;
		out.Jump = false;
		{
			bool cantjump = false;
			// This will report air on the first frame of unducking and report ground on the last one.
			if ((*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(player) + offDucking))
			    && GetFlagsDucking())
			{
				cantjump = true;
			}

			auto djt = (*reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offDuckJumpTime));
			djt -= vars.Frametime * 1000;
			djt = std::max(0.f, djt);
			float flDuckMilliseconds = std::max(0.0f, 1000.f - djt);
			float flDuckSeconds = flDuckMilliseconds * 0.001f;
			if (flDuckSeconds > 0.2)
			{
				djt = 0;
			}
			if (djt > 0)
			{
				cantjump = true;
			}

			if (!cantjump && vars.OnGround)
			{
				if (tas_strafe_lgagst.GetBool())
				{
					bool jump = LgagstJump(pl, vars);
					if (jump)
					{
						vars.OnGround = false;
						out.Jump = true;
						jumped = true;
					}
				}

				if (ORIG_GetButtonBits(thisptr, 0, 0) & IN_JUMP)
				{
					vars.OnGround = false;
					jumped = true;
				}
			}
		}

		Friction(pl, vars.OnGround, vars);

		if (tas_strafe_vectorial
		        .GetBool()) // Can do vectorial strafing even with locked camera, provided we are not jumping
			Strafe::StrafeVectorial(pl,
			                        vars,
			                        jumped,
			                        GetFlagsDucking(),
			                        type,
			                        dir,
			                        tas_strafe_yaw.GetFloat(),
			                        va[YAW],
			                        out,
			                        yawChanged);
		else if (!yawChanged) // not changing yaw, can do regular strafe
			Strafe::Strafe(pl,
			               vars,
			               jumped,
			               GetFlagsDucking(),
			               type,
			               dir,
			               tas_strafe_yaw.GetFloat(),
			               va[YAW],
			               out,
			               btns,
			               usingButtons);

		// This bool is set if strafing should occur
		if (out.Processed)
		{
			if (out.Jump && !pl.Ducking && pl.DuckPressed && tas_strafe_autojb.GetBool())
			{
				forceUnduck = true;
			}

			if (out.Jump && tas_strafe_jumptype.GetInt() > 0)
			{
				aim::SetJump();
			}

			forceJump = out.Jump;
			*reinterpret_cast<float*>(pCmd + offForwardmove) = out.ForwardSpeed;
			*reinterpret_cast<float*>(pCmd + offSidemove) = out.SideSpeed;
			va[YAW] = static_cast<float>(out.Yaw);
		}
	}

	EngineSetViewAngles(va);
	OngroundSignal(IsGroundEntitySet());
	TickSignal();
}

_declspec(naked) void ClientDLL::HOOKED_HDTF_MiddleOfViewBobFuncStart()
{
	__asm {
		pushad;
		pushfd;
	}

	if (y_spt_hdtf_viewbob.GetBool())
		_asm
		{
			popfd;
			popad;
			jmp clientDLL.ORIG_HDTF_MiddleOfViewBobFuncStart;
		}
	else 
		_asm
	    {
		    popfd;
		    popad;
		    jmp clientDLL.ORIG_HDTF_MiddleOfViewBobFuncEnd;
	    }
}

_declspec(naked) void ClientDLL::HOOKED_HDTF_MiddleOfViewRollFunc()
{
	__asm {
		pushad;
		pushfd;
	}

	if (y_spt_hdtf_viewroll.GetBool()) _asm
	{
		popfd;
		popad;
		jmp clientDLL.ORIG_HDTF_MiddleOfViewRollFunc;
	}
	else _asm
	{
		popfd;
		popad;
		jmp clientDLL.ORIG_HDTF_MiddleOfViewRollFunc_JumpTo;
	}
}

void __fastcall ClientDLL::HOOKED_CreateMove_Func(void* thisptr,
                                                  int edx,
                                                  int sequence_number,
                                                  float input_sample_frametime,
                                                  bool active)
{
	auto m_pCommands = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(thisptr) + offM_pCommands);
	pCmd = m_pCommands + sizeofCUserCmd * (sequence_number % 90);

	ORIG_CreateMove(thisptr, edx, sequence_number, input_sample_frametime, active);

	pCmd = 0;
}

void __fastcall ClientDLL::HOOKED_CViewRender__OnRenderStart_Func(void* thisptr, int edx)
{
	ORIG_CViewRender__OnRenderStart(thisptr, edx);

	if (!_viewmodel_fov || !_y_spt_force_90fov.GetBool())
		return;

	float* fov = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 52);
	float* fovViewmodel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 56);
	*fov = 90;
	*fovViewmodel = _viewmodel_fov->GetFloat();
}

void ClientDLL::HOOKED_CViewRender__RenderView_Func(void* thisptr,
                                                    int edx,
                                                    void* cameraView,
                                                    int nClearFlags,
                                                    int whatToDraw)
{
#ifndef SSDK2007
	ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
#else
	if (g_OverlayRenderer.shouldRenderOverlay())
	{
		g_OverlayRenderer.modifyView(static_cast<CViewSetup*>(cameraView), renderingOverlay);
		if (renderingOverlay)
		{
			g_OverlayRenderer.modifySmallScreenFlags(nClearFlags, whatToDraw);
		}
		else
		{
			g_OverlayRenderer.modifyBigScreenFlags(nClearFlags, whatToDraw);
		}
	}

	ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
#endif
}

void ClientDLL::HOOKED_CViewRender__Render_Func(void* thisptr, int edx, void* rect)
{
#ifndef SSDK2007
	screenRect = rect;
	ORIG_CViewRender__Render(thisptr, edx, rect);
#else
	renderingOverlay = false;
	screenRect = rect;
	if (!g_OverlayRenderer.shouldRenderOverlay())
	{
		ORIG_CViewRender__Render(thisptr, edx, rect);
	}
	else
	{
		ORIG_CViewRender__Render(thisptr, edx, rect);

		renderingOverlay = true;
		Rect_t rec = g_OverlayRenderer.getRect();
		screenRect = &rec;
		ORIG_CViewRender__Render(thisptr, edx, &rec);
		renderingOverlay = false;
	}
#endif
}

void ClientDLL::HOOKED_ResetToneMapping(float value)
{
	if (!y_spt_disable_tone_map_reset.GetBool())
		clientDLL.ORIG_ResetToneMapping(value);
}
