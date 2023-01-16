#pragma once

namespace utils
{
	bool DoesGameLookLikePortal();
	bool DoesGameLookLikeDMoMM();
	bool DoesGameLookLikeHLS();
	bool DoesGameLookLikeBMSRetail();
	bool DoesGameLookLikeBMSLatest();
	bool DoesGameLookLikeBMSMod();
	bool DoesGameLookLikeEstranged();
	int DateToBuildNumber(const char* date_str);
	int GetBuildNumber();
	void StartBuildNumberSearch();
} // namespace utils