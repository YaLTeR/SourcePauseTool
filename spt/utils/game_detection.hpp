#pragma once

namespace utils
{
	bool DoesGameLookLikePortal();
	bool DoesGameLookLikeDMoMM();
	bool DoesGameLookLikeHLS();
	bool DoesGameLookLikeBMS();
	bool DoesGameLookLikeBMSMod();
	bool DoesGameLookLikeEstranged();
	int DateToBuildNumber(const char* date_str);
	int GetBuildNumber();
	void StartBuildNumberSearch();
} // namespace utils