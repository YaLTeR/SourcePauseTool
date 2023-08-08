#pragma once
#include "stdafx.hpp"

#include <SPTLib\memutils.hpp>
#include <SPTLib\patterns.hpp>
#include <array>
#include <cstddef>

/*
* The purpose of this file is purely for archiving old and unused patterns
* to possibly speed up development in the future. If you or a loved one
* finds a pattern that you later decide you won't use, then you may be
* entitled to financial compensation. The way you get that compensation is
* by putting your patterns here for safekeeping. In code, don't reference
* anything in this file - if you need to use any patterns from here then
* cut and paste them into your feature.
*/

namespace patterns
{
	/****************************** ENGINE ******************************/

	PATTERNS(SV_Frame,
	         "5135",
	         "8B 0D ?? ?? ?? ?? 83 EC 08 85 C9 74 10 8B 44 24 0C 84 C0 74 08 8B 11 50 8B 42 78 FF D0 83 3D");
	PATTERNS(GetScreenAspect, "5135", "83 EC 0C A1 ?? ?? ?? ?? F3");
	PATTERNS(CVRenderView__VGui_Paint, "5135", "E8 ?? ?? ?? ?? 8B 10 8B 52 34 8B C8 FF E2 CC CC");
	PATTERNS(
	    CEngineTrace__PointOutsideWorld,
	    "5135",
	    "8B 44 24 04 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? C1 E0 04 83 C4 04 66 83 7C 08 04 FF 0F 94 C0 C2 04 00");
	PATTERNS(CEngineVGui__Paint,
	         "dmomm",
	         "6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 64 89 25 ?? ?? ?? ?? 83 EC 1C 56 6A 04");
	PATTERNS(CChangelevel__ChangeLevelNow,
	         "BMS-0.9",
	         "55 8B EC B8 24 20 00 00 E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 33 C5");
	PATTERNS(
	    LoadSaveGame,
	    "BMS-0.9",
	    "55 8B EC 8B 0D ?? ?? ?? ?? 56 8B 75 ?? 56 8B 01 8B 40 ?? FF D0 84 C0 75 ?? 56 68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 83 C4 08 5E 5D C3");

	// Search for "FinishedMapLoad", one of two references to address
	PATTERNS(CL_FullyConnected,
	         "BMS-0.9",
	         "55 8B EC 81 EC 14 04 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 ?? 56 68",
	         "5135",
	         "55 8B EC 83 E4 C0 83 EC 34 53 56 57 E8");
	PATTERNS(SCR_BeginLoadingPlaque,
	         "5135",
	         "80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 10 8B C8",
	         "4044",
	         "80 3D ?? ?? ?? ?? 00 75 ?? 6A 01 C6 05 ?? ?? ?? ?? 01 E8 ?? ?? ?? ?? 6A 01",
	         "SDK-2013",
	         "80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 6A 00 8B C8 8B 10 FF 92",
	         "7671541",
	         "80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 6A 00 8B C8 8B 10 FF 92");
	// Search for "Disconnect: %s\n", func below string
	PATTERNS(Host_Disconnect,
	         "BMS-0.9",
	         "55 8B EC 80 3D ?? ?? ?? ?? 00 75 ?? FF 75 ?? B9 ?? ?? ?? ?? FF 75 ?? E8",
	         "5135",
	         "80 3D ?? ?? ?? ?? 00 75 ?? 8B 44 24 ?? 50 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 6A 00 E8");
	// Search for "Only the server may changelevel\n"
	PATTERNS(Host_Changelevel,
	         "BMS-0.9",
	         "55 8B EC 81 EC 7C 03 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 ?? 83 3D ?? ?? ?? ?? 02");
	// Search for "Save file %s can\'t be found!\n"
	PATTERNS(SCR_EndLoadingPlaque,
	         "BMS-0.9",
	         "80 3D ?? ?? ?? ?? 00 74 ?? E8 ?? ?? ?? ?? 8B C8 8B 10 FF 52 ?? 6A 00 E8",
	         "5135",
	         "80 3D ?? ?? ?? ?? 00 74 ?? E8 ?? ?? ?? ?? 8B 10 8B C8 8B 42 ?? FF D0 6A 00",
	         "4044",
	         "53 33 DB 38 1D ?? ?? ?? ?? 74 ?? E8 ?? ?? ?? ?? 8B 10 8B C8 FF 52 ?? 53",
	         "SDK-2013",
	         "80 3D ?? ?? ?? ?? 00 74 ?? E8 ?? ?? ?? ?? 8B C8 8B 10 FF 52 ?? 6A 00 E8",
	         "7671541",
	         "80 3D ?? ?? ?? ?? 00 74 ?? E8 ?? ?? ?? ?? 8B C8 8B 10 FF 52 ?? 6A 00 E8");

	/****************************** CLIENT ******************************/

	PATTERNS(MiddleOfCAM_Think,
	         "5135",
	         "8B 54 24 74 8B 4C 24 78 83 EC 0C 8B C4 89 10 8B 94 24 88 00 00 00 89 48 04 89 50 08 E8",
	         "4104",
	         "D9 41 2C D8 E1 DE F2 DE C1 8B 0D ?? ?? ?? ?? D9 5C 24 14 83 79 30 00 0F 84 09 01 00 00 E8",
	         "1910503",
	         "F3 0F 7E 45 ?? 8B 4D D0 83 EC 0C 8B C4 66 0F D6 00 89 48 08 E8 ?? ?? ?? ?? 50 B9 ?? ?? ?? ?? E8",
	         "2257546",
	         "8B 45 D4 83 EC 0C 8B D4 89 02 8B 45 D8 89 42 04 8B 45 DC 89 42 08 E8 ?? ?? ?? ?? 50 B9");
	PATTERNS(CHLClient__CanRecordDemo,
	         "5135",
	         "E8 ?? ?? ?? ?? 85 C0 74 11 E8 ?? ?? ?? ?? 8B 10 8B C8 8B 92 84 00 00 00 FF E2");
	PATTERNS(
	    UTIL_TraceLine,
	    "5135",
	    "55 8B EC 83 E4 F0 8B 45 0C 8B 4D 08 83 EC 5C 56 50 51 8D 4C 24 18 E8 ?? ?? ?? ?? 8B 75 18 8B 45 14 8B 0D ?? ?? ?? ?? 8B 11 8B 52 10 56 50");
	PATTERNS(UTIL_TracePlayerBBox,
	         "5135",
	         "55 8B EC 83 E4 F0 83 EC 5C 56 8B F1 8B 06 8B 50 24 FF D2 50 8B 06 8B 50 20 8B CE FF D2");
	PATTERNS(CGameMovement__CanUnDuckJump,
	         "5135",
	         "83 EC 60 53 55 8B 6C 24 6C 56 57 8B F1 8B 7E 08 8B 87 98 00 00 00 8B 1E 81 C7 98 00 00 00");
	PATTERNS(CalcAbsoluteVelocity,
	         "5135",
	         "83 EC 1C 57 8B F9 F7 87 ?? 01 00 00 00 10 00 00 0F 84 76 01 00 00 53 56 8D 9F ?? 04 00 00 FF 15",
	         "1910503",
	         "55 8B EC 83 EC 20 57 8B F9 F7 87 ?? ?? ?? ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 53 56 8D 9F",
	         "2257546",
	         "55 8B EC 83 EC 20 57 8B F9 F7 87 ?? ?? ?? ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 56 8D B7");
	PATTERNS(
	    CViewRender__Render,
	    "5135",
	    "81 EC 98 00 00 00 53 56 57 6A 04 6A 00 68 ?? ?? ?? ?? 6A 00 8B F1 8B ?? ?? ?? ?? ?? 68 ?? ?? ?? ?? FF ?? ?? ?? ?? ?? 8B BC 24 A8 00 00 00 8B 4F 04",
	    "7462488",
	    "55 8B EC 81 EC 30 01 00 00 53");
	PATTERNS(CViewRender__QueueOverlayRenderView,
	         "3420",
	         "8B 44 24 ?? 56 8B F1 50 8D 8E ?? ?? ?? ??",
	         "7462488",
	         "55 8B EC 56 FF 75 ?? 8B F1 8D 8E ?? ?? ?? ??");
	PATTERNS(CDebugViewRender__Draw2DDebuggingInfo, "5135", "A1 ?? ?? ?? ?? 81 EC 9C 00 00 00");
	PATTERNS(DecodeUserCmdFromBuffer, "dmomm", "81 EC BC 00 00 00 56 8B F1 8D 4C 24 ?? E8 ?? ?? ?? ?? 8D 44 24 ??");
	PATTERNS(ClientModeShared__CreateMove,
	         "5135",
	         "E8 ?? ?? ?? ?? 85 C0 75 ?? B0 01 C2 08 00 8B 4C 24 ?? D9 44 24 ?? 8B 10",
	         "1910503",
	         "55 8B EC E8 ?? ?? ?? ?? 85 C0 75 ?? B0 01 5D C2 08 00 8B 4D ?? 8B 10",
	         "7197370",
	         "55 8B EC E8 ?? ?? ?? ?? 8B C8 85 C9 75 ?? B0 01 5D C2 08 00 8B 01");
	PATTERNS(OnRenderStart,
	         "5135-portal1",
	         "56 8B 35 ?? ?? ?? ?? 8B 06 8B 50 64 8B CE FF D2 8B 0D ?? ?? ?? ??",
	         "7462488-portal1",
	         "55 8B EC 83 EC 10 53 56 8B 35 ?? ?? ?? ?? 8B CE 57 8B 06 FF 50 64 8B 0D ?? ?? ?? ??");

	/****************************** SERVER ******************************/

	PATTERNS(
	    PlayerRunCommand,
	    "5135",
	    "56 8B 74 24 08 57 8B F9 33 C9 39 8F 18 0A 00 00 88 8F 74 0B 00 00 75 1B D9 46 0C D9 9F 08 0A 00 00 D9 46 10 D9 9F 0C 0A 00 00 D9 46 14 D9 9F 10",
	    "4104",
	    "56 8B 74 24 08 57 8B F9 33 C9 39 8F D8 09 00 00 88 8F 34 0B 00 00 75 1B D9 46 0C D9 9F C8 09 00 00 D9 46 10 D9 9F CC 09 00 00 D9 46 14 D9 9F D0",
	    "5339",
	    "55 8B EC 56 8B 75 08 57 8B F9 33 C0 88 87 B0 0B 00 00 39 87 54 0A 00 00 75 1B D9 46 0C D9 9F 44 0A 00 00 D9 46 10 D9 9F 48 0A 00 00 D9 46 14 D9",
	    "2257546",
	    "55 8B EC 56 8B F1 57 8B 7D 08 83 BE 54 0A 00 00 00 C6 86 B0 0B 00 00 00 75 1B D9 47 0C D9 9E 44 0A 00 00 D9 47 10 D9 9E 48 0A 00 00 D9 47 14 D9",
	    "2257546-hl1",
	    "55 8B EC 56 8B F1 57 8B 7D 08 83 BE 24 0A 00 00 00 C6 86 80 0B 00 00 00 75 1B D9 47 0C D9 9E 14 0A 00 00 D9 47 10 D9 9E 18 0A 00 00 D9 47 14 D9",
	    "bms",
	    "55 8B EC 56 8B 75 08 57 8B F9 33 C0 88 87 80 0B 00 00 39 87 24 0A 00 00 75 1B D9 46 0C D9 9F 14 0A 00 00 D9 46 10 D9 9F 18 0A 00 00 D9 46 14 D9",
	    "estranged",
	    "55 8B EC 56 8B 75 08 57 8B F9 33 C0 88 87 B4 0B 00 00 39 87 58 0A 00 00 75 1B D9 46 0C D9 9F 48 0A 00 00 D9 46 10 D9 9F 4C 0A 00 00 D9 46 14 D9",
	    "estranged-second",
	    "55 8B EC 56 8B 75 08 57 8B F9 33 C0 88 87 B4 0B 00 00 39 87 58 0A 00 00 75 1B D9 46 0C D9 9F 48 0A 00 00 D9 46 10 D9 9F 4C 0A 00 00 D9 46 14 D9",
	    "2707",
	    "8B 81 ?? ?? ?? ?? 8B 54 24 04 53 33 DB 3B C3 88 99 ?? ?? ?? ?? 75 1B 8B 42 0C 89 81",
	    "4044",
	    "8B 54 24 04 53 33 DB 39 99 ?? ?? ?? ?? 88 99 ?? ?? ?? ?? 75 1B 8B 42 0C 89 81 ?? ?? ?? ?? 8B 42 10",
	    "6879",
	    "55 8B EC 56 8B 75 08 57 8B F9 33 C0 88 87 ?? ?? ?? ?? 39 87 ?? ?? ?? ?? 75 1B D9 46 0C D9 9F",
	    "missinginfo1_6",
	    "55 8B EC 56 8B 75 08 57 8B F9 33 C9 88 8F ?? ?? ?? ?? 39 8F ?? ?? ?? ?? 75 1B D9 46 0C D9 9F");
	PATTERNS(
	    GetStepSoundVelocities,
	    "5135",
	    "F6 81 00 01 00 00 02 75 24 80 B9 32 01 00 00 09 74 1B D9 ?? ?? ?? ?? ?? 8B ?? ?? ?? 8B ?? ?? ?? D9 18 D9 ?? ?? ?? ?? ?? D9 19 C2 08 00");
	PATTERNS(
	    PerformFlyCollisionResolution,
	    "5135",
	    "56 8B F1 8B 4E 04 8A 81 33 01 00 00 0F B6 D0 83 FA 01 57 8B 7C 24 0C 77 2B 3C 01 75 0E D9 ?? ?? ?? ?? ?? D8 ?? ?? ?? ?? ?? EB 02");
	PATTERNS(
	    CBaseEntity__SetCollisionGroup,
	    "5135",
	    "56 8B F1 8B 86 ?? ?? ?? ?? 3B 44 24 08 8D 8E A4 01 00 00 74 11 8D 54 24 08 52 E8 ?? ?? ?? ?? 8B CE E8 ?? ?? ?? ?? 5E C2 04 00");
	PATTERNS(
	    CreateEntityByName,
	    "5135",
	    "56 8B 74 24 0C 83 FE FF 57 8B 7C 24 0C 74 27 8B ?? ?? ?? ?? ?? 8B 01 8B 50 54 56 FF D2 85 C0 A3 ?? ?? ?? ?? 75 10 56 57 68 ?? ?? ?? ?? FF ?? ?? ?? ?? ??");
	PATTERNS(
	    CRestore__ReadAll,
	    "5135",
	    "53 8B 5C 24 08 56 8B 74 24 10 8B 46 0C 85 C0 57 8B F9 74 12 50 56 53 ?? ?? ?? ?? ?? 85 C0 75 06 5F 5E 5B C2 08 00 8B 4E 04 8B 16 8B 07 51 8B 4E 08 52 8B 50 0C");
	PATTERNS(
	    CRestore__DoReadAll,
	    "5135",
	    "53 8B 5C 24 0C 55 8B 6C 24 0C 56 57 8B 7C 24 1C 8B 47 0C 85 C0 8B F1 74 13 50 53 55 E8 ?? ?? ?? ?? 85 C0 75 07 5F 5E 5D 5B C2 0C 00 8B 4F 04 8B 17 8B 06 51 8B 4F 08 52 8B 50 0C");
	PATTERNS(
	    DispatchSpawn,
	    "5135",
	    "53 55 56 8B 74 24 10 85 F6 57 0F ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 8B 03 8B 50 64 8B CB FF D2 8B 06 8B 50 08 8B CE FF D2 8B ?? ?? ?? ?? ?? 8B 28 8B 01 8B ?? ?? ?? ?? ?? 6A 00 6A 03 FF D2 88 44 24 14");
	PATTERNS(
	    AllocPooledString,
	    "5135",
	    "8B 44 24 08 85 C0 74 24 80 38 00 74 1F 50 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 74 05 80 38 00 75 02 33 C0 8B 4C 24 04 89 01 8B C1 C3 8B 44 24 04 C7 00 00 00 00 00 C3");
	PATTERNS(
	    FindPooledString,
	    "5135",
	    "8B 44 24 08 50 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 74 0E 80 38 00 74 09 8B 4C 24 04 89 01 8B C1 C3 8B 44 24 04 33 C9 89 08 C3");
	PATTERNS(CGameMovement__DecayPunchAngle,
	         "5135",
	         "83 EC 0C 56 8B F1 8B 56 ?? D9 82 ?? ?? ?? ?? 8D 8A ?? ?? ?? ??");

	/****************************** VPHYSICS ******************************/

	PATTERNS(CPhysicsObject__GetPosition,
	         "5135",
	         "8B 49 08 81 EC 80 00 00 00 8D 04 24 50 E8 ?? ?? ?? ?? 8B 84 24 84 00 00 00 85 C0",
	         "1910503",
	         "55 8B EC 8B 49 08 81 EC 80 00 00 00 8D 45 80 50 E8 ?? ?? ?? ?? 8B 45 08 85 C0",
	         "7462488",
	         "55 8B EC 8B 49 ?? 8D 45 ?? 81 EC 80 00 00 00 50 E8 ?? ?? ?? ?? 8B 45 ??");

	PATTERNS(CPhysicsCollision__CreateDebugMesh,
	         "5135",
	         "83 EC 10 8B 4C 24 14 8B 01 8B 40 08 55 56 57 33 ED 8D 54 24 10 52",
	         "1910503",
	         "55 8B EC 83 EC 14 8B 4D 08 8B 01 8B 40 08 53 56 57 33 DB 8D 55 EC",
	         "7462488",
	         "55 8B EC 83 EC 18 8B 4D ?? 8D 55 ??");

	/****************************** DATACACHE ******************************/

	PATTERNS(CMDLCache__BeginMapLoad, "BMS-0.9", "55 8B EC 83 EC 08 53 56 8B F1 57 89 75");

} // namespace patterns
