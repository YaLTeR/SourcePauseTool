#pragma once

#include <SPTLib\memutils.hpp>
#include <SPTLib\patterns.hpp>
#include <array>
#include <cstddef>

namespace patterns
{
	namespace engine
	{
		PATTERNS(
		    SpawnPlayer,
		    "5135",
		    "83 EC 14 80 3D ?? ?? ?? ?? 00 56 8B F1 74 ?? 6A 00 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB 23 8B 0D ?? ?? ?? ?? 8B 01 8B 96 ?? ?? ?? ?? 8B 40 0C 52 FF D0 8B 8E ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 C4 04 8B 46 0C 8B 56 04 8B 52 74 83 C0 01",
		    "5339",
		    "55 8B EC 83 EC 14 80 3D ?? ?? ?? ?? 00 56 8B F1 74 ?? 6A 00 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB 23 8B 0D ?? ?? ?? ?? 8B 01 8B 96 ?? ?? ?? ?? 8B 40 0C 52 FF D0 8B 8E ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 C4 04 8B 46 0C 8B 56 04 8B 52 74 40",
		    "4104",
		    "83 EC 14 80 3D ?? ?? ?? ?? 00 56 8B F1 74 ?? 6A 00 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB 23 8B 0D ?? ?? ?? ?? 8B 01 8B 96 ?? ?? ?? ?? 8B 40 0C 52 FF D0 8B 8E ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 C4 04 8B 46 0C 8B 56 04 8B 52 70 83 C0 01",
		    "2257546",
		    "55 8B EC 83 EC 14 80 3D ?? ?? ?? ?? 00 56 8B F1 74 ?? 6A 00 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB 1F 8B 0D ?? ?? ?? ?? FF B6 ?? ?? ?? ?? 8B 01 FF 50 0C FF B6 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04 8B 46 0C 8D 4E 04 40",
		    "2707",
		    "64 A1 00 00 00 00 6A FF 68 ?? ?? ?? ?? 50 64 89 25 00 00 00 00 83 EC 14 80 3D ?? ?? ?? ?? 00 56 8B F1 74 0E 6A 00 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB 21 8B 0D",
		    "6879",
		    "55 8B EC 83 EC 14 80 3D ?? ?? ?? ?? ?? 56 8B F1 74 0E 6A 00 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB 23");
		PATTERNS(
		    SV_ActivateServer,
		    "5135",
		    "83 EC 08 57 8B 3D ?? ?? ?? ?? 68 ?? ?? ?? ?? FF D7 83 C4 04 E8 ?? ?? ?? ?? 8B 10",
		    "5339",
		    "55 8B EC 83 EC 0C 57 8B 3D ?? ?? ?? ?? 68 ?? ?? ?? ?? FF D7 83 C4 04 E8 ?? ?? ?? ?? 8B 10",
		    "2257546",
		    "55 8B EC 83 EC 0C 53 8B 1D ?? ?? ?? ?? 57 68 ?? ?? ?? ?? FF D3 83 C4 04 E8 ?? ?? ?? ?? 6A 0B",
		    "2707",
		    "6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04 E8 ?? ?? ?? ?? 8B 10 6A 0B 8B C8 FF 92",
		    "6879",
		    "55 8B EC 83 EC 08 56 57 8B 3D ?? ?? ?? ?? 68 ?? ?? ?? ?? FF D7 83 C4 04 E8 ?? ?? ?? ?? 8B 10 8B C8");
		PATTERNS(
		    FinishRestore,
		    "5135",
		    "81 EC A4 06 00 00 33 C0 55 8B E9 8D 8C 24 20 01 00 00 89 84 24 08 01 00 00",
		    "5339",
		    "55 8B EC 81 EC A4 06 00 00 33 C0 53 8B D9 8D 8D 74 F9 FF FF 89 85 5C F9 FF FF",
		    "2257546",
		    "55 8B EC 81 EC A4 06 00 00 33 C0 53 8B D9 89 85 5C F9 FF FF 8D 8D 74 F9 FF FF",
		    "2707",
		    "55 8B EC 83 E4 F8 81 EC 8C 06 00 00 55 56 57 8B E9 80 BD ?? ?? ?? ?? 00 0F 84 C0 00 00 00",
		    "BMS-Retail",
		    "55 8B EC 81 EC A8 06 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 33 C0 53 8B D9 89 85 58 F9 FF FF",
		    "4044",
		    "55 8B EC 83 E4 F8 81 EC 8C 06 00 00 55 56 57 8B E9 80 BD ?? 01 00 00 00 0F 84 BE 00 00 00 33 C0",
		    "6879",
		    "55 8B EC 81 EC ?? ?? ?? ?? 53 8B D9 8D 8D ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 BB ?? ?? ?? ?? ?? 75 1B");
		PATTERNS(
		    SetPaused,
		    "5135",
		    "83 EC 14 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 06 8B 50 ?? 8B CE FF D2 84 C0 74 ?? 8A 44 24 1C 8B 16 8B 92 80 00 00 00",
		    "5339",
		    "55 8B EC 83 EC 14 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 06 8B 50 ?? 8B CE FF D2 84 C0 74 ?? 8A 45 08 8B 16 8B 92 80 00 00 00",
		    "4104",
		    "83 EC 14 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 06 8B 50 ?? 8B CE FF D2 84 C0 74 ?? 8A 44 24 1C 8B 16 8B 52 7C 33 C9 84 C0",
		    "2257546",
		    "55 8B EC 83 EC 14 56 8B F1 8B 06 8B 40 ?? FF D0 84 C0 74 ?? 8B 06 8B CE 8B 40 ?? FF D0 84 C0 74 ?? 8A 4D 08 33 C0 84 C9 88 4D FC 6A 00",
		    "2707",
		    "64 A1 00 00 00 00 6A FF 68 ?? ?? ?? ?? 50 64 89 25 00 00 00 00 83 EC 14 56 8B F1 8B 06 FF 50 ?? 84 C0 74 59 8B 16 8B CE FF 52",
		    "6879",
		    "55 8B EC 83 EC 14 56 8B F1 83 7E 04 03 74 0B 8B 06 8B 50 68 FF D2 84 C0 74 46 8B 06 8B 50 54 8B CE");
		PATTERNS(
		    MiddleOfSV_InitGameDLL,
		    "4104",
		    "8B 0D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? FF ?? D9 15 ?? ?? ?? ?? DD 05 ?? ?? ?? ?? DB F1 DD 05 ?? ?? ?? ?? 77 08 D9 CA DB F2 76 1F D9 CA",
		    "4044",
		    "8B 0D ?? ?? ?? ?? 8B 01 83 C4 14 FF 50 24 D9 15 ?? ?? ?? ?? DC 1D ?? ?? ?? ?? DF E0 F6 C4 05 7B 13");
		PATTERNS(
		    _Host_RunFrame,
		    "5135",
		    "55 8B EC 83 EC 1C 53 56 57 33 FF 80 3D ?? ?? ?? ?? 00 74 17 E8 ?? ?? ?? ?? 83 F8 FE 74 0D 68");
		PATTERNS(
		    SV_Frame,
		    "5135",
		    "8B 0D ?? ?? ?? ?? 83 EC 08 85 C9 74 10 8B 44 24 0C 84 C0 74 08 8B 11 50 8B 42 78 FF D0 83 3D");
		PATTERNS(
		    Record,
		    "2707",
		    "81 EC 08 01 00 00 E8 ?? ?? ?? ?? 83 F8 02 74 1E E8 ?? ?? ?? ?? 83 F8 03 74 14 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04 81 C4 08 01 00 00 C3 53 32 DB 88 5C 24 04 E8",
		    "5135",
		    "81 EC 08 01 00 00 83 ?? ?? ?? ?? ?? ?? 75 15 68 ?? ?? ?? ?? FF ?? ?? ?? ?? ?? 83 C4 04 81 C4 08 01 00 00 C3");
		PATTERNS(VGui_Paint, "5135", "E8 ?? ?? ?? ?? 8B 10 8B 52 34 8B C8 FF E2 CC CC");
		PATTERNS(
		    CEngineTrace__PointOutsideWorld,
		    "5135",
		    "8B 44 24 04 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? C1 E0 04 83 C4 04 66 83 7C 08 04 FF 0F 94 C0 C2 04 00");
		PATTERNS(
		    Host_AccumulateTime,
		    "5135",
		    "51 F3 0F 10 ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? F3 0F 11 ?? ?? ?? ?? ?? 8B 01 8B 50");
		PATTERNS(StopRecording,
		         "5135",
		         "56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 86 ?? ?? ?? ?? 85 C0 57",
		         "1910503",
		         "55 8B EC 51 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 0F ?? ?? ?? ?? ?? 8B 86 ?? ?? ?? ?? 57");
		PATTERNS(SetSignonState,
		         "5135",
		         "CC 56 8B F1 8B ?? ?? ?? ?? ?? 8B 01 8B 50 ?? FF D2 84 C0 75 ?? 8B",
		         "1910503",
		         "CC 55 8B EC 56 8B F1 8B ?? ?? ?? ?? ?? 8B 01 8B 50 ?? FF D2 84");
		PATTERNS(Stop,
		         "5135",
		         "83 3D ?? ?? ?? ?? 01 75 ?? 8B 0D ?? ?? ?? ?? 8B 01 8B ?? ?? FF D2 84 C0 75 ?? C7",
		         "1910503",
		         "83 3D ?? ?? ?? ?? 01 75 ?? 8B 0D ?? ?? ?? ?? 8B 01 8B ?? ?? FF D2 84 C0 75 ?? 68");
		PATTERNS(DebugDrawPhysCollide,
		         "5135",
		         "81 EC 38 02 00 00 53 55 33 DB 39 9C 24 48 02 00 00 56 57 75 24",
		         "1910503",
		         "55 8B EC 81 EC 3C 02 00 00 53 33 DB 56 57 39 5D 0C 75 20");
	} // namespace engine

	namespace client
	{
		PATTERNS(
		    DoImageSpaceMotionBlur,
		    "5135",
		    "A1 ?? ?? ?? ?? 81 EC 8C 00 00 00 83 78 30 00 0F 84 F3 06 00 00 8B 0D ?? ?? ?? ?? 8B 11 8B 82 80 00 00 00 FF D0",
		    "5339",
		    "55 8B EC A1 ?? ?? ?? ?? 83 EC 7C 83 78 30 00 0F 84 4A 08 00 00 8B 0D ?? ?? ?? ?? 8B 11 8B 82 80 00 00 00 FF D0",
		    "4104",
		    "A1 ?? ?? ?? ?? 81 EC 8C 00 00 00 83 78 30 00 0F 84 EE 06 00 00 8B 0D ?? ?? ?? ?? 8B 11 8B 42 7C FF D0",
		    "2257546",
		    "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC A1 ?? ?? ?? ?? 81 EC 98 00 00 00 83 78 30 00",
		    "1910503",
		    "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC A1 ?? ?? ?? ?? 81 EC ?? ?? ?? ?? 83 78 30 00 56 57 0F 84 ?? ?? ?? ?? 8B 0D",
		    "missinginfo1_6",
		    "55 8B EC A1 ?? ?? ?? ?? 81 EC ?? ?? ?? ?? 83 78 30 00 0F 84 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 11");
		PATTERNS(
		    CheckJumpButton_client,
		    "5135",
		    "83 EC 18 56 8B F1 8B 4E 04 80 B9 74 0F 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 83 C4 18 C3 D9 EE D8 91 6C 10 00 00",
		    "4104",
		    "83 EC 18 56 8B F1 8B 4E 08 80 B9 3C 0F 00 00 00 74 0E 8B 76 04 83 4E 28 02 32 C0 5E 83 C4 18 C3 D9 EE D8 91 30 10 00 00",
		    "5339",
		    "55 8B EC 83 EC 1C 56 8B F1 8B 4E 04 80 B9 24 10 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 20 11 00 00",
		    "2257546",
		    "55 8B EC 83 EC 14 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5",
		    "2257546-hl1",
		    "55 8B EC 51 56 8B F1 57 8B 7E 04 85 FF 74 2B 8B 07 8B CF 8B 80 ?? ?? ?? ?? FF D0 84 C0 74 1B 6A 00",
		    "2707",
		    "83 EC 10 53 56 8B F1 8B 4E 08 8A 81 ?? ?? ?? ?? 84 C0 74 0F 8B 76 04 83 4E 28 02 5E 32 C0 5B 83 C4 10",
		    "4044-episodic",
		    "83 EC 10 56 8B F1 8B 4E 08 80 B9 ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? D9 05 ?? ?? ?? ?? D9 81",
		    "1910503",
		    "55 8B EC 83 EC 14 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5",
		    "6879",
		    "55 8B EC 83 EC 0C 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 07 32 C0 5E 8B E5 5D C3 53 BB",
		    "missinginfo1_4_7",
		    "55 8B EC 83 EC 3C 56 89 4D D8 8B 45 D8 8B 48 08 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 C8 85 C9",
		    "missinginfo1_6",
		    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5");

		PATTERNS(
		    HudUpdate,
		    "5135",
		    "51 A1 ?? ?? ?? ?? D9 40 10 56 83 EC 08 D9 54 24 0C DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? 8B 4C 24 0C 51 B9",
		    "5339",
		    "55 8B EC 51 A1 ?? ?? ?? ?? D9 40 10 56 83 EC 08 D9 55 FC DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? 8B 4D 08 51 B9",
		    "2257546",
		    "55 8B EC 51 A1 ?? ?? ?? ?? 56 83 EC 08 D9 40 10 D9 55 FC DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? FF 75 08 B9",
		    "bms",
		    "55 8B EC 51 A1 ?? ?? ?? ?? F3 0F 10 40 10 56 F3 0F 11 45 FC 83 EC 08 0F 5A C0 F2 0F 11 04 24 E8 ?? ?? ?? ?? 8B C8",
		    "2707",
		    "51 A1 ?? ?? ?? ?? 8B 48 10 89 4C 24 00 E8 ?? ?? ?? ?? D9 44 24 00 83 EC 08 DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8",
		    "dmomm",
		    "51 A1 ?? ?? ?? ?? 8B 48 10 89 0C 24 D9 04 24 83 EC 08 DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8",
		    "6879",
		    "55 8B EC 83 EC 24 A1 ?? ?? ?? ?? F3 0F 10 40 ?? 53 56 F3 0F 11 45 ?? 83 EC 08 0F 5A C0 F2 0F 11 04 24",
		    "missinginfo1_4_7",
		    "55 8B EC 83 EC 08 89 4D F8 A1 ?? ?? ?? ?? 8B 48 10 89 4D FC E8 ?? ?? ?? ?? D9 45 FC 83 EC 08 DD 1C 24",
		    "hl1movement",
		    "55 8B EC 51 A1 ?? ?? ?? ?? 56 83 EC 08 F3 0F 10 40 10 F3 0F 11 45 FC 0F 5A C0 F2 0F 11 04 24 E8 ?? ?? ?? ?? 8B C8");
		PATTERNS(
		    GetButtonBits,
		    "5135",
		    "55 56 8B E9 B1 03 33 F6 84 0D ?? ?? ?? ?? 57 74 05 BE 00 00 02 00 8B 15 ?? ?? ?? ?? F7 C2 00 00 02 00 B8 FD FF FF",
		    "5339",
		    "55 8B EC 56 33 F6 F6 05 ?? ?? ?? ?? 03 74 05 BE 00 00 02 00 8B 15 ?? ?? ?? ?? B8 FD FF FF FF F7 C2 00 00 02 00 74",
		    "2257546",
		    "55 8B EC 51 8B 15 ?? ?? ?? ?? B8 00 00 02 00 53 56 33 F6 8B D9 F6 C2 03 B9 FD FF FF FF 57 0F 45 F0 BF FC FF FF FF",
		    "2707",
		    "51 A0 ?? ?? ?? ?? B2 03 84 C2 C7 44 24 00 00 00 00 00 74 08 C7 44 24 00 00 00 02 00 8B 0D ?? ?? ?? ?? F7 C1 00 00 02 00",
		    "4044-episodic",
		    "56 B0 03 33 F6 84 05 ?? ?? ?? ?? 74 05 BE ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? F7 C2 ?? ?? ?? ?? B9",
		    "6879",
		    "55 8B EC 83 EC 0C 56 8B F1 8B 0D ?? ?? ?? ?? 8B 01 8B 90 ?? ?? ?? ?? 57 89 75 F8 FF D2 8B F8 C7 45",
		    "missinginfo1_4_7",
		    "55 8B EC 83 EC 08 89 4D F8 C7 45 ?? ?? ?? ?? ?? 83 7D 08 00 0F 95 C0 50 68 ?? ?? ?? ?? 8B 0D");
		PATTERNS(
		    AdjustAngles,
		    "5135",
		    "83 EC 0C D9 44 24 10 56 51 D9 1C 24 8B F1 E8 ?? ?? ?? ?? D9 54 24 14 D9 EE DE D9 DF E0 F6 C4 01 0F 84 90 00 00 00 8B 0D",
		    "5135-hl2",
		    "83 EC 0C D9 44 24 10 56 51 D9 1C 24 8B F1 E8 ?? ?? ?? ?? D9 54 24 14 D9 EE DE D9 DF E0 F6 C4 01 74 5D 8B 0D",
		    "4104",
		    "83 EC 0C D9 44 24 10 56 51 D9 1C 24 8B F1 E8 ?? ?? ?? ?? D9 54 24 14 D9 EE DE D9 DF E0 F6 C4 01 74 58 8B 0D",
		    "5339",
		    "55 8B EC A1 ?? ?? ?? ?? F3 0F 10 4D 08 0F 57 D2 83 EC 0C 83 78 30 00 56 8B F1 74 1F F3 0F 10 46 1C 0F 2F D0",
		    "2257546",
		    "55 8B EC A1 ?? ?? ?? ?? 83 EC 0C F3 0F 10 4D 08 0F 57 D2 83 78 30 00 56 8B F1 74 1F F3 0F 10 46 1C 0F 2F D0",
		    "5377866",
		    "55 8B EC A1 ?? ?? ?? ?? 83 EC 0C F3 0F 10 4D 08 F3 0F 10 15 ?? ?? ?? ?? 83 78 30 00 56 8B F1 74 1F F3 0F 10 46 1C",
		    "BMS_Retail",
		    "55 8B EC 83 EC 0C D9 45 08 56 51 D9 1C 24 8B F1 E8 ?? ?? ?? ?? D9 55 08 D9 EE DF F1 DD D8 73 50 8B 0D ?? ?? ?? ?? 8D 55 F4");
		PATTERNS(
		    CreateMove,
		    "5135",
		    "83 EC 14 53 D9 EE 55 56 57 8B F9 8B 4C 24 28 B8 B7 60 0B B6 F7 E9 03 D1 C1 FA 06 8B C2 C1 E8 1F 03 D0 6B D2 5A 8B C1 2B C2 8B F0 6B C0 58 03 87",
		    "4104",
		    "83 EC 14 53 D9 EE 55 56 57 8B F9 8B 4C 24 28 B8 B7 60 0B B6 F7 E9 03 D1 C1 FA 06 8B C2 C1 E8 1F 03 C2 6B C0 5A 8B F1 2B F0 6B F6 54 03 B7 C4 00",
		    "1910503",
		    "55 8B EC 83 EC 50 53 8B D9 8B 4D 08 B8 ?? ?? ?? ?? F7 E9 03 D1 C1 FA 06 8B C2 C1 E8 1F 03 D0 0F 57 C0",
		    "2257546",
		    "55 8B EC 83 EC 54 53 56 8B 75 08 B8 ?? ?? ?? ?? F7 EE 57 03 D6 8B F9 C1 FA 06 8B CE 8B C2 C1 E8 1F",
		    "2257546-hl1",
		    "55 8B EC 83 EC 50 53 8B 5D 08 B8 ?? ?? ?? ?? F7 EB 56 03 D3 C1 FA 06 8B C2 C1 E8 1F 03 C2 8B D3",
		    "BMS-Retail",
		    "55 8B EC 83 EC 54 53 56 8B 75 08 B8 ?? ?? ?? ?? F7 EE 57 03 D6 8B F9 C1 FA 06 8B CE 8B C2 C1 E8 1F 03 C2 6B C0 5A");
		PATTERNS(
		    DecodeUserCmdFromBuffer,
		    "5135",
		    "83 ec 54 33 c0 d9 ee 89 44 24 40 d9 54 24 0c 89 44 24 44 d9 54 24 10 89 44 24 48 d9 54 24 14 89 44 24 50 d9 54 24 18 89 44 24 04");
		PATTERNS(
		    MiddleOfCAM_Think,
		    "5135",
		    "8B 54 24 74 8B 4C 24 78 83 EC 0C 8B C4 89 10 8B 94 24 88 00 00 00 89 48 04 89 50 08 E8",
		    "4104",
		    "D9 41 2C D8 E1 DE F2 DE C1 8B 0D ?? ?? ?? ?? D9 5C 24 14 83 79 30 00 0F 84 09 01 00 00 E8",
		    "1910503",
		    "F3 0F 7E 45 ?? 8B 4D D0 83 EC 0C 8B C4 66 0F D6 00 89 48 08 E8 ?? ?? ?? ?? 50 B9 ?? ?? ?? ?? E8",
		    "2257546",
		    "8B 45 D4 83 EC 0C 8B D4 89 02 8B 45 D8 89 42 04 8B 45 DC 89 42 08 E8 ?? ?? ?? ?? 50 B9");
		PATTERNS(
		    GetGroundEntity,
		    "5135",
		    "8B 81 EC 01 00 00 83 F8 FF 74 20 8B 15 ?? ?? ?? ?? 8B C8 81 E1 FF 0F 00 00 C1 E1 04 8D 4C",
		    "4104",
		    "8B 81 E8 01 00 00 83 F8 FF 74 20 8B 15 ?? ?? ?? ?? 8B C8 81 E1 FF 0F 00 00 C1 E1 04 8D 4C 11 04",
		    "1910503",
		    "8B 81 50 02 00 00 83 F8 FF 74 1F 8B 15 ?? ?? ?? ?? 8B C8 81 E1 ?? ?? ?? ?? 03 C9 8D 4C CA 04 C1 E8 0C",
		    "2257546",
		    "8B 91 50 02 00 00 83 FA FF 74 1D A1 ?? ?? ?? ?? 8B CA 81 E1 ?? ?? ?? ?? C1 EA 0C 03 C9 39 54 C8 08");
		PATTERNS(
		    CalcAbsoluteVelocity,
		    "5135",
		    "83 EC 1C 57 8B F9 F7 87 ?? 01 00 00 00 10 00 00 0F 84 76 01 00 00 53 56 8D 9F ?? 04 00 00 FF 15",
		    "1910503",
		    "55 8B EC 83 EC 20 57 8B F9 F7 87 ?? ?? ?? ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 53 56 8D 9F",
		    "2257546",
		    "55 8B EC 83 EC 20 57 8B F9 F7 87 ?? ?? ?? ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 56 8D B7");
		PATTERNS(
		    CViewRender__OnRenderStart,
		    "2707",
		    "83 EC 38 56 8B F1 8B 0D ?? ?? ?? ?? D9 41 28 57 D8 1D ?? ?? ?? ?? DF E0 F6 C4 05 7A 35 A1 ?? ?? ?? ?? D9 40 28 8B 0D",
		    "missinginfo1_4_7",
		    "55 8B EC 83 EC 24 89 4D E0 B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 05 7A 5A");
		PATTERNS(
		    CViewRender__RenderView,
		    "5135",
		    "55 8B EC 83 E4 F8 81 EC 24 01 00 00 53 56 57 8B F9 8D 8F 20 01 00 00 89 7C 24 24 E8",
		    "3420",
		    "55 8B EC 83 E4 F8 81 EC 24 01 00 00 53 8B 5D 08 56 57 8B F9 8D 8F 94 00 00 00 53 89 7C 24 28 89 4C 24 34 E8");
		PATTERNS(
		    CViewRender__Render,
		    "5135",
		    "81 EC 98 00 00 00 53 56 57 6A 04 6A 00 68 ?? ?? ?? ?? 6A 00 8B F1 8B ?? ?? ?? ?? ?? 68 ?? ?? ?? ?? FF ?? ?? ?? ?? ?? 8B BC 24 A8 00 00 00 8B 4F 04");
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
		PATTERNS(
		    UTIL_TraceRay,
		    "5135",
		    "8B 44 24 10 8B 4C 24 0C 83 EC 10 56 6A 00 50 51 8D 4C 24 10 E8 ?? ?? ?? ?? 8B 74 24 28 8B 0D ?? ?? ?? ?? 8B 11 8B 52 10",
		    "3420",
		    "8B 44 24 10 8B 4C 24 0C 83 EC 0C 56 50 51 8D 4C 24 0C E8 ?? ?? ?? ?? 8B 74 24 24 8B 0D ?? ?? ?? ?? 8B 11 8B 52 10");
		PATTERNS(
		    CViewEffects__Fade,
		    "dmomm-4104-5135",
		    "51 56 6A 14 8B F1 E8 ?? ?? ?? ?? 8B 54 24 10 8B C8 0F B7 02 89 44 24 10 83 C4 04 89 4C 24 04 DB 44 24 0C",
		    "5377866",
		    "55 8B EC 51 56 57 6A 14 89 4D FC E8 ?? ?? ?? ?? 8B 7D 08 8B F0");
		PATTERNS(CViewEffects__Shake,
		         "4104-5135",
		         "56 8B 74 24 08 8B 06 85 C0 57 8B F9 74 05 83 F8 04 75 54 83 7F 24 20 7D 4E 6A 28",
		         "5377866",
		         "55 8B EC 56 8B 75 ?? 57 8B F9 8B 06 85 C0 74 ?? 83 F8 04");
		PATTERNS(
		    CHudDamageIndicator__GetDamagePosition,
		    "5135",
		    "83 EC 18 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 08 89 4C 24 0C 8B 50 04 6A 00 89 54 24 14 8B 40 08 6A 00 8D 4C 24 08 51 8D 54 24 18 52 89 44 24 24",
		    "1910503",
		    "55 8B EC 83 EC 18 56 8B F1 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 7E 00 6A 00 6A 00 8D 4D F4 66 0F D6 45 E8");
		PATTERNS(
		    ResetToneMapping,
		    "5135",
		    "8B 0D ?? ?? ?? ?? 8B 01 8B 90 7C 01 00 00 56 FF D2 8B F0 85 F6 74 09 8B 06 8B 50 08 8B CE FF D2 8B 06 D9 44 24 08",
		    "1910503",
		    "55 8B EC 8B 0D ?? ?? ?? ?? 8B 01 8B 90 88 01 00 00 56 FF D2 8B F0 85 F6 74 09 8B 06 8B 50 08 8B CE FF D2 8B 06 F3 0F 10 45 08",
		    "5377866",
		    "55 8B EC 8B 0D ?? ?? ?? ?? 56 8B 01 FF 90 88 01 00 00 8B F0 85 F6 74 07 8B 06 8B CE FF 50 08 8B 06 D9 45 08",
		    "BMS-Retail",
		    "55 8B EC 8B 0D ?? ?? ?? ?? 56 8B 01 FF 90 ?? ?? ?? ?? 8B F0 85 F6 74 ?? 8B 06 8B CE FF 50 ?? 8B 06 D9 45 ?? 51 8B CE",
		    "te120",
		    "55 8B EC 8B 0D ?? ?? ?? ?? 8B 01 8B 90 ?? ?? ?? ?? 56 FF D2 8B F0 85 F6 74 ?? 8B 06 8B 50 ?? 8B CE FF D2 8B 06");
		PATTERNS(CRendering3dView__DrawTranslucentRenderables,
		         "5135",
		         "55 8B EC 83 EC 34 53 8B D9 8B 83 94 00 00 00 8B 13 56 8D B3 94 00 00 00",
		         "1910503",
		         "55 8B EC 81 EC 9C 00 00 00 53 56 8B F1 8B 86 E8 00 00 00 8B 16 57 8D BE E8 00 00 00");
	} // namespace client

	namespace server
	{
		PATTERNS(
		    CheckJumpButton,
		    "5135",
		    "83 EC 1C 56 8B F1 8B 4E 04 80 B9 04 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 70 0D 00 00",
		    "4104",
		    "83 EC 1C 56 8B F1 8B 4E 08 80 B9 C4 09 00 00 00 74 0E 8B 76 04 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 30 0D 00 00",
		    "5339",
		    "55 8B EC 83 EC 20 56 8B F1 8B 4E 04 80 B9 40 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 AC 0D 00 00",
		    "2257546",
		    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 40 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 AC 0D 00 00",
		    "2257546-hl1",
		    "55 8B EC 51 56 8B F1 57 8B 7E 04 85 FF 74 10 8B 07 8B CF 8B 80 ?? ?? ?? ?? FF D0 84 C0 75 02 33 FF",
		    "1910503",
		    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 40 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B",
		    "bms",
		    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 10 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 E8 08 F9 FF FF 84 C0 75 F0",
		    "estranged",
		    "55 8B EC 83 EC ?? 56 8B F1 8B 4E 04 80 B9 44 0A 00 00 00 74 0E 8B ?? 08 83 ?? 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 B0 0D 00 00 0F 57 C0 0F 2E",
		    "2707",
		    "83 EC 10 53 56 8B F1 8B 4E 08 8A 81 ?? ?? ?? ?? 84 C0 74 0F 8B 76 04 83 4E ?? 02 5E 32 C0 5B 83 C4 10 C3 D9 81 30 0C 00 00 D8 1D",
		    "BMS-Retail-3",
		    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 9C 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0",
		    "BMS-Retail",
		    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 78 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5 5D C3 8B 06 8B 80 44 01 00 00 FF D0 84 C0 75",
		    "BMS-Retail-2",
		    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 84 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B",
		    "2949",
		    "83 EC 14 56 8B F1 8B 4E 08 80 B9 30 09 00 00 00 0F 85 E1 00 00 00 D9 05 ?? ?? ?? ?? D9 81 70 0C 00 00",
		    "dmomm",
		    "81 EC ?? ?? ?? ?? 56 8B F1 8B 4E 10 80 B9 ?? ?? ?? ?? ?? 74 11 8B 76 0C 83 4E 28 02 32 C0 5E 81 C4",
		    "4044-episodic",
		    "83 EC 14 56 8B F1 8B 4E 08 80 B9 ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? D9 05 ?? ?? ?? ?? D9 81",
		    "6879",
		    "55 8B EC 83 EC 0C 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 07 32 C0 5E 8B E5 5D C3 53 BB",
		    "missinginfo1_4_7",
		    "55 8B EC 83 EC 44 56 89 4D D0 8B 45 D0 8B 48 08 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 C8 85 C9",
		    "te120",
		    "55 8B EC 83 EC 20 56 8B F1 8B 4E 04 80 B9 48 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 B4 0D 00 00",
		    "icemod2013",
		    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 50 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 D8 0D 00 00",
		    "missinginfo1_6",
		    "55 8B EC 83 EC 1C 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5");
		PATTERNS(
		    FinishGravity,
		    "bms",
		    "8B 51 04 F3 0F 10 82 7C 0D 00 00 0F 57 C9 0F 2E C1 9F F6 C4 44 7A 51 F3 0F 10 82 28 02 00 00 0F 2E C1 9F F6 C4 44",
		    "2707",
		    "8B 51 08 D9 82 ?? ?? ?? ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 44 7A 4D D9 82 ?? ?? ?? ?? D8 1D",
		    "4044",
		    "8B 51 08 D9 05 ?? ?? ?? ?? D9 82 ?? ?? ?? ?? DA E9 DF E0 F6 C4 44 7A 4F D9 05 ?? ?? ?? ?? D9 82",
		    "4104",
		    "D9 EE 8B 51 08 D8 92 ?? ?? ?? ?? DF E0 F6 C4 44 7A 3F D8 9A ?? ?? ?? ?? DF E0 F6 C4 44 7B 08 D9 82",
		    "5135",
		    "D9 EE 8B 51 04 D8 92 ?? ?? ?? ?? DF E0 F6 C4 44 7A 3F D8 9A ?? ?? ?? ?? DF E0 F6 C4 44 7B 08 D9 82",
		    "2257546",
		    "55 8B EC 51 0F 57 C9 57 8B F9 8B 57 04 F3 0F 10 82 ?? ?? ?? ?? 0F 2E C1 9F F6 C4 44 7A 47 F3 0F 10 82",
		    "6879",
		    "8B 51 04 F3 0F 10 8A ?? ?? ?? ?? 0F 57 C0 0F 2E C8 9F F6 C4 44 0F 8A ?? ?? ?? ?? F3 0F 10 8A",
		    "missinginfo1_4_7",
		    "55 8B EC 83 EC 0C 89 4D F8 8B 45 F8 8B 48 08 D9 05 ?? ?? ?? ?? D9 81 ?? ?? ?? ?? DA E9 DF E0 F6 C4 44",
		    "missinginfo1_6",
		    "8B 51 04 D9 82 ?? ?? ?? ?? D9 EE D9 C0 DD EA DF E0 DD D9 F6 C4 44 7A 3F D9 82 ?? ?? ?? ?? DA E9");
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
		    CheckStuck,
		    "5135",
		    "81 EC 80 00 00 00 56 8B F1 C7 44 24 04 FF FF FF FF E8 ?? ?? ?? ?? 8B 46 08 8B 16 8B 92 BC 00 00 00 8D 4C 24 30 51 05 98 00 00 00 6A 08 50 8D 44",
		    "2707",
		    "81 EC ?? ?? ?? ?? 53 56 8B F1 57 8D 4C 24 6C C7 44 24 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? E8",
		    "4104",
		    "81 EC ?? ?? ?? ?? 56 8B F1 C7 44 24 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 56 04 8B 06 8D 4C 24 30 51",
		    "ghosting",
		    "55 8B EC 81 EC 80 00 00 00 56 8B F1 C7 45 FC FF FF FF FF E8 ?? ?? ?? ?? 8B 46 08 8B 16 8D 4D 80 51 05 98 00 00 00 6A 08 50 8D 45 EC 50 8B CE FF",
		    "1910503",
		    "55 8B EC 81 EC 80 00 00 00 57 8B F9 E8 ?? ?? ?? ?? 85 C0 0F 84 47 02 00 00 56 8B 77 04 8B 06 8B",
		    "BMS-Retail",
		    "55 8B EC 81 EC 84 00 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 56 8B F1 C7 45 A0 FF FF FF FF E8",
		    "BMS-Retail-2",
		    "55 8b ec 81 ec 84 00 00 00 a1 ?? ?? ?? ?? 33 c5 89 45 fc 56 8b f1 c7 45 a4 ff ff ff ff e8 ?? ?? ?? ?? 8b 46 08",
		    "4044-episodic",
		    "81 EC ?? ?? ?? ?? 56 57 8B F1 C7 44 24 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 4E 04 8D 44 24 34 50 6A 08",
		    "2257546",
		    "55 8B EC 81 EC ?? ?? ?? ?? 56 8B F1 C7 45 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 46 08 8D 4D 80 8B 16",
		    "6879",
		    "55 8B EC 81 EC ?? ?? ?? ?? 53 56 57 8B F1 C7 45 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 46 08 8B 16 8B 92",
		    "missinginfo1_4_7",
		    "55 8B EC 81 EC ?? ?? ?? ?? 89 8D ?? ?? ?? ?? 8D 4D EC E8 ?? ?? ?? ?? 8D 4D 80 E8 ?? ?? ?? ?? 8D 8D",
		    "missinginfo1_6",
		    "55 8B EC 81 EC ?? ?? ?? ?? 56 8B F1 C7 45 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 46 08 8B 16 8B 92");
		PATTERNS(
		    MiddleOfSlidingFunction,
		    "dmomm",
		    "8B 16 8B CE FF 92 ?? ?? ?? ?? 8B 08 89 4C 24 1C 8B 50 04 89 54 24 20 8B 40 08 8D 4C 24 10 51 8D 54 24 20")
		PATTERNS(
		    CHL2_Player__HandleInteraction,
		    "5135",
		    "8B 44 24 04 3B 05 ?? ?? ?? ?? 74 36 3B 05 ?? ?? ?? ?? 75 15 83 A1 ?? ?? ?? ?? ?? 6A 00 6A 02 E8 ?? ?? ?? ?? B0 01 C2 0C 00")
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
		PATTERNS(TracePlayerBBoxForGround,
		         "5135",
		         "55 8B EC 83 E4 F0 81 EC 84 00 00 00 53 56 8B 75 24 8B 46 0C D9 46 2C 8B 4E 10");
		PATTERNS(TracePlayerBBoxForGround2,
		         "5135",
		         "55 8B EC 83 E4 F0 8B 4D 18 8B 01 8B 50 08 81 EC 84 00 00 00 53 56 57 FF D2");
		PATTERNS(CGameMovement__TracePlayerBBox, "5135", "55 8B EC 83 E4 F0 83 EC 5C 56 8B F1 8B 06 8B 50 24");
		PATTERNS(CPortalGameMovement__TracePlayerBBox,
		         "5135",
		         "55 8B EC 83 E4 F0 81 EC C4 00 00 00 53 56 8B F1 8B 46 08 83 C0 04 8B 00");
		PATTERNS(
		    CGameMovement__GetPlayerMins,
		    "4104",
		    "8B 41 ?? 8B 88 ?? ?? ?? ?? C1 E9 03 F6 C1 01 8B 0D ?? ?? ?? ?? 8B 11 74 09 8B 42 ?? FF D0 83 C0 48 C3");
		PATTERNS(
		    CGameMovement__GetPlayerMaxs,
		    "4104",
		    "8B 41 ?? 8B 88 ?? ?? ?? ?? C1 E9 03 F6 C1 01 8B 0D ?? ?? ?? ?? 8B 11 74 09 8B 42 ?? FF D0 83 C0 54 C3");
		PATTERNS(SetPredictionRandomSeed, "5135", "8B 44 24 ?? 85 C0 75 ?? C7 05 ?? ?? ?? ?? FF FF FF FF");
		PATTERNS(CGameMovement__DecayPunchAngle,
		         "5135",
		         "83 EC 0C 56 8B F1 8B 56 ?? D9 82 ?? ?? ?? ?? 8D 8A ?? ?? ?? ??");
		PATTERNS(MiddleOfTeleportTouchingEntity,
		         "5135",
		         "8B 80 24 27 00 00 8B CD 8B A9 24 27 00 00 89 44 24 3C");
		PATTERNS(EndOfTeleportTouchingEntity, "5135", "E8 E3 CC DB FF 8D 8C 24 B8 00 00 00 E8 17 45 F5 FF");
	} // namespace server

	namespace vguimatsurface
	{
		PATTERNS(
		    StartDrawing,
		    "5135",
		    "55 8B EC 83 E4 C0 83 EC 38 80 ?? ?? ?? ?? ?? ?? 56 57 8B F9 75 57 8B ?? ?? ?? ?? ?? C6 ?? ?? ?? ?? ?? ?? FF ?? 8B 10 6A 00 8B C8 8B 42 20");
		PATTERNS(
		    FinishDrawing,
		    "5135",
		    "56 6A 00 E8 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 8B 01 8B ?? ?? ?? ?? ?? 83 C4 04 FF D2 8B F0 85 F6 74 09 8B 06 8B 50 08 8B CE FF D2");
	} // namespace vguimatsurface

	namespace inputsystem
	{
		PATTERNS(CInputSystem__SleepUntilInput,
		         "5135",
		         "8B 44 24 ?? 85 C0 7D ??",
		         "5377866-BMS_Retail",
		         "55 8B EC 8B 45 ?? 83 CA FF");
	} // namespace inputsystem

	namespace vphysics
	{
		PATTERNS(MiddleOfRecheck_ov_element,
		         "5135",
		         "C6 05 ?? ?? ?? ?? 01 83 EE 01 3B 74 24 28 7D D3 8B 4C 24 38",
		         "1910503",
		         "C6 05 ?? ?? ?? ?? 01 4E 3B 75 F0 7D D3 8B 8D DC FD FF FF");
		PATTERNS(
		    GetShadowPosition,
		    "5135",
		    "81 EC ?? ?? ?? ?? 8B 41 ?? 8B 40 ?? 8B 40",
		    "5377866",
		    "55 8B EC 81 EC ?? ?? ?? ?? 8B 41 08 8B 40 08 8B 50",
		    "BMS-Retail",
		    "55 8B EC 81 EC ?? ?? ?? ?? A1 ?? ?? ?? ?? 33 C5 89 45 FC 8B 41 ?? 56 8B 75 ?? 57 8B 40 ?? 8B 7D ?? 8B 50");
		PATTERNS(CPhysicsCollision__CreateDebugMesh,
		         "5135",
		         "83 EC 10 8B 4C 24 14 8B 01 8B 40 08 55 56 57 33 ED 8D 54 24 10 52",
		         "1910503",
		         "55 8B EC 83 EC 14 8B 4D 08 8B 01 8B 40 08 53 56 57 33 DB 8D 55 EC");
		PATTERNS(CPhysicsCollision__DestroyDebugMesh,
		         "5135",
		         "8B 44 24 08 50 E8 ?? ?? ?? ?? 59 C2 08 00",
		         "1910503",
		         "55 8B EC 8B 45 0C 50 E8 ?? ?? ?? ?? 83 C4 04 5D C2 08 00");
		PATTERNS(CPhysicsObject__GetPosition,
		         "5135",
		         "8B 49 08 81 EC 80 00 00 00 8D 04 24 50 E8 ?? ?? ?? ?? 8B 84 24 84 00 00 00 85 C0",
		         "1910503",
		         "55 8B EC 8B 49 08 81 EC 80 00 00 00 8D 45 80 50 E8 ?? ?? ?? ?? 8B 45 08 85 C0");
	} // namespace vphysics

} // namespace patterns
