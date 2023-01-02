#include "stdafx.hpp"
#include "..\spt-serverplugin.hpp"
#include "SPTLib\sptlib.hpp"
#include "SPTLib\MemUtils.hpp"
#include "interfaces.hpp"
#include "thirdparty\kmp-cpp.hpp"
#include <atomic>
#include <thread>
#include <future>

#ifdef SSDK2013
#include "command.hpp"
#endif

namespace utils
{
	bool DoesGameLookLikePortal()
	{
#ifndef OE
		if (g_pCVar)
		{
			if (g_pCVar->FindCommand("upgrade_portalgun"))
				return true;

			return false;
		}

		if (interfaces::engine)
		{
			auto game_dir = interfaces::engine->GetGameDirectory();
			return (GetFileName(Convert(game_dir)) == L"portal");
		}
#endif

		return false;
	}

	bool DoesGameLookLikeDMoMM()
	{
#ifdef OE
		if (g_pCVar)
		{
			if (g_pCVar->FindVar("mm_xana_fov"))
				return true;
		}
#endif

		return false;
	}

	bool DoesGameLookLikeHLS()
	{
		if (g_pCVar)
		{
			if (g_pCVar->FindVar("hl1_ref_db_distance"))
				return true;
		}

		return false;
	}

	bool DoesGameLookLikeBMS()
	{
		if (g_pCVar)
		{
			if (g_pCVar->FindVar("bm_announcer"))
				return true;
		}

		return false;
	}

	bool DoesGameLookLikeBMSMod()
	{
		if (DoesGameLookLikeBMS())
		{
			return false;
		}

		if (g_pCVar)
		{
			if (g_pCVar->FindVar("bm_eds_crash"))
				return true;
		}

		return false;
	}

	bool DoesGameLookLikeEstranged()
	{
#ifndef OE
		if (interfaces::engine)
		{
			auto game_dir = interfaces::engine->GetGameDirectory();
			return (GetFileName(Convert(game_dir)) == L"estrangedact1");
		}
#endif

		return false;
	}

	static std::future<int> BuildResult;

	int GetBuildNumber()
	{
		static int BuildNumber = -1;
		static bool HasBuildNumber = false;

		if (!HasBuildNumber)
		{
			BuildNumber = BuildResult.get();
			HasBuildNumber = true;
		}

		return BuildNumber;
	}

	int DateToBuildNumber(const char* date_str)
	{
		const char* months[] = {
		    "Jan",
		    "Feb",
		    "Mar",
		    "Apr",
		    "May",
		    "Jun",
		    "Jul",
		    "Aug",
		    "Sep",
		    "Oct",
		    "Nov",
		    "Dec",
		};

		int monthDays[] = {
		    31,
		    28,
		    31,
		    30,
		    31,
		    30,
		    31,
		    31,
		    30,
		    31,
		    30,
		    31,
		};

		int m = 0, d = 0, y = 0;

		while (m < 11)
		{
			auto month = months[m];
			if (strstr(date_str, month) == date_str)
			{
				break;
			}

			d += monthDays[m];
			m += 1;
		}

		if (date_str[4] == ' ')
		{
			d += (date_str[5] - '0') - 1;
		}
		else
		{
			d += (date_str[4] - '0') * 10 + (date_str[5] - '0') - 1;
		}

		y = std::atoi(date_str + 7) - 1900;

		int build_num;

		build_num = (y - 1) * 365.25;
		build_num += d;
		if (y % 4 == 0 && m > 1)
			build_num += 1;
		build_num -= 35739;
		return build_num;
	}

#ifndef SSDK2013
	void StartBuildNumberSearch()
	{
		BuildResult = std::async(
		    std::launch::async,
		    []()
		    {
			    void* handle;
			    uint8_t* moduleStart;
			    uint8_t* moduleEnd;
			    size_t moduleSize;
			    int build_num = -1;

			    if (MemUtils::GetModuleInfo(L"engine.dll",
			                                &handle,
			                                reinterpret_cast<void**>(&moduleStart),
			                                &moduleSize))
			    {
				    moduleEnd = moduleStart + moduleSize;
				    const char* BUILD_STRING = "Exe build:";
				    const uint8_t* beginNeedle = reinterpret_cast<const uint8_t*>(BUILD_STRING);
				    const uint8_t* endNeedle = beginNeedle + strlen(BUILD_STRING);

				    int match = kmp::match_first(beginNeedle, endNeedle, moduleStart, moduleEnd);

				    if (match >= 0)
				    {
					    const char* wholeString =
					        reinterpret_cast<const char*>(moduleStart) + match;
					    DevMsg("Found date string: %s\n", wholeString);
					    const char* date_str = wholeString + 20;
					    build_num = DateToBuildNumber(date_str);
				    }
				    else
				    {
					    Warning(
					        "Was unable to find date string! Build information not available\n");
				    }
			    }

			    return build_num;
		    });
	}
#else
	int GetBuildNumberViaVersionCommand(uint8_t* moduleStart, uint8_t* moduleEnd)
	{
		ConCommand_guts* versionCommand = reinterpret_cast<ConCommand_guts*>(g_pCVar->FindCommand("version"));
		int build_num = -1;

		if (versionCommand)
		{
			uint8_t* buildNumPtrPtr = reinterpret_cast<uint8_t*>(versionCommand->m_fnCommandCallback) + 3;

			if (buildNumPtrPtr >= moduleStart && buildNumPtrPtr <= moduleEnd)
			{
				uint8_t* buildNumPtr = *reinterpret_cast<uint8_t**>(buildNumPtrPtr);

				if (buildNumPtr >= moduleStart && buildNumPtr <= moduleEnd)
				{
					build_num = *reinterpret_cast<int*>(buildNumPtr);
				}
			}
		}

		if (build_num == -1)
		{
			Warning("Build information not available!\n");
		}

		return build_num;
	}

	void StartBuildNumberSearch()
	{
		BuildResult = std::async(std::launch::async,
		                         []()
		                         {
			                         void* handle;
			                         uint8_t* moduleStart;
			                         uint8_t* moduleEnd;
			                         size_t moduleSize;
			                         int build_num = -1;

			                         if (MemUtils::GetModuleInfo(L"engine.dll",
			                                                     &handle,
			                                                     reinterpret_cast<void**>(&moduleStart),
			                                                     &moduleSize))
			                         {
				                         moduleEnd = moduleStart + moduleSize;
				                         build_num =
				                             GetBuildNumberViaVersionCommand(moduleStart, moduleEnd);
			                         }

			                         return build_num;
		                         });
	}
#endif

} // namespace utils