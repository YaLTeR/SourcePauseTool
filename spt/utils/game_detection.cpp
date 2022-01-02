#include "stdafx.h"
#include "..\spt-serverplugin.hpp"
#include "SPTLib\sptlib.hpp"
#include "interfaces.hpp"

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
} // namespace utils