#include "stdafx.hpp"

#ifdef _WIN32

#include "thirdparty\json.hpp"
#include "game_detection.hpp"
#include "interfaces.hpp"
#include "file.hpp"
#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\spt-serverplugin.hpp"

#include <Windows.h>
#include <dbghelp.h>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

#define CURL_STATICLIB
extern "C"
{
#include <curl\curl.h>
}

#ifdef SSDK2007
#define ASSET_NAME "spt.dll"
#elif SSDK2013
#define ASSET_NAME "spt-2013.dll"
#elif BMS
#define ASSET_NAME "spt-bms.dll"
#elif OE
#define ASSET_NAME "spt-oe.dll"
#endif

// Update spt
class Updater : public FeatureWrapper<Updater>
{
public:
	enum
	{
		UPDATER_ERROR,
		UPDATER_UP_TO_DATE,
		UPDATER_AHEAD,
		UPDATER_OUTDATED,
	};

	struct ReleaseInfo
	{
		std::string name;
		std::string url;
		std::string body;
		int build;
	} release;

	int CheckUpdate();
	bool ReloadPlugin();
	void UpdatePlugin();

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void LoadFeature() override;

private:
	CURL* curl;
	curl_slist* slist;

	std::string sptPath;

	bool Prepare(const char* url, int timeOut);
	bool Download(const char* url, const char* path);
	std::string Request(const char* url);
	bool FetchReleaseInfo();
};

static Updater spt_updater;

CON_COMMAND(y_spt_check_update, "Check the release information of spt.")
{
	int res = spt_updater.CheckUpdate();
	switch (res)
	{
	case Updater::UPDATER_UP_TO_DATE:
		Msg("Already up-to-date.\n");
		break;
	case Updater::UPDATER_AHEAD:
		Msg("Ahead of latest release version.\n");
		break;
	case Updater::UPDATER_OUTDATED:
		Msg("Found newer version. Update with `y_spt_update`, or download at '%s'\n",
		    spt_updater.release.url.c_str());
		break;
	}
}

CON_COMMAND(y_spt_update, "Check and install available update for spt. Usage: y_spt_update [force]")
{
	if (args.ArgC() > 2)
	{
		Msg("Usage: y_spt_update [force]\n");
		return;
	}
	bool force = false;

	if (args.ArgC() == 2 && std::strcmp(args.Arg(1), "force") == 0)
		force = true;

	int res = spt_updater.CheckUpdate();

	if ((force && res != Updater::UPDATER_ERROR) || (res == Updater::UPDATER_OUTDATED))
	{
		spt_updater.UpdatePlugin();
	}
	else if (res == Updater::UPDATER_UP_TO_DATE)
	{
		Msg("Already up-to-date.\n");
	}
	else if (res == Updater::UPDATER_AHEAD)
	{
		Msg("Ahead of latest release version.\n");
		Msg("Use `y_spt_update force` to install the latest release version.\n");
	}
}

CON_COMMAND(y_spt_reload, "Reload the plugin.")
{
	spt_updater.ReloadPlugin();
}

bool Updater::ShouldLoadFeature()
{
	return true;
}

static std::string GetPluginPath()
{
	SymInitialize(GetCurrentProcess(), 0, true);
	DWORD module = SymGetModuleBase(GetCurrentProcess(), (DWORD)&GetPluginPath);
	char filename[MAX_PATH + 1];
	GetModuleFileNameA((HMODULE)module, filename, MAX_PATH);
	SymCleanup(GetCurrentProcess());
	return std::string(filename);
}

void Updater::LoadFeature()
{
	sptPath = GetPluginPath();
	InitCommand(y_spt_check_update);
	InitCommand(y_spt_update);
#ifndef OE
	InitCommand(y_spt_reload);
#endif
}

bool Updater::Prepare(const char* url, int timeout)
{
	if (!curl)
	{
		curl = curl_easy_init();
		if (!curl)
		{
			DevMsg("Cannot init curl.\n");
			return false;
		}
		slist = curl_slist_append(slist, "Accept: application/vnd.github.v3+json");
		slist = curl_slist_append(slist, "User-Agent: SourcePauseTool");
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

	return true;
}

bool Updater::Download(const char* url, const char* path)
{
	if (!Prepare(url, 60))
		return false;

	FILE* fp = fopen(path, "wb");
	if (!fp)
	{
		DevMsg("Cannot open path '%s'\n", path);
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	CURLcode res = curl_easy_perform(curl);

	fclose(fp);

	if (res != CURLE_OK)
	{
		DevMsg("Error (%d)\n", res);
		return false;
	}

	return true;
}

std::string Updater::Request(const char* url)
{
	if (!Prepare(url, 10))
	{
		return "";
	}

	curl_easy_setopt(
	    curl,
	    CURLOPT_WRITEFUNCTION,
	    +[](void* ptr, size_t sz, size_t nmemb, std::string* data) -> size_t
	    {
		    data->append((char*)ptr, sz * nmemb);
		    return sz * nmemb;
	    });

	std::string response;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK)
	{
		DevMsg("Error (%d)\n", res);
		return "";
	}

	return response;
}

static int GetPluginBuildNumber()
{
	// Use the way Source Engine calculates the build number to calculate SPT "build number"
	int pluginBuildNumber = 0;
	char sptVersion[] = SPT_VERSION;
	sptVersion[11] = '\0';
	pluginBuildNumber = utils::DateToBuildNumber(sptVersion);
	return pluginBuildNumber;
}

bool Updater::FetchReleaseInfo()
{
	std::string data = Request("https://api.github.com/repos/YaLTeR/SourcePauseTool/releases/latest");
	if (data == "")
		return false;

	nlohmann::json res = nlohmann::json::parse(data);
	release.name = res["tag_name"];

	if (release.name == "")
	{
		DevMsg("Cannot get tag_name.\n");
		return false;
	}

	// Get release build number
	std::string date = res["created_at"];
	if (date == "")
	{
		DevMsg("Cannot get created_at.\n");
		return false;
	}

	std::tm tm = {};
	std::stringstream ss(date.substr(0, 10));
	ss >> std::get_time(&tm, "%Y-%m-%d");
	std::mktime(&tm);
	if (ss.fail())
		return false;

	int m = tm.tm_mon;
	int d = tm.tm_yday;
	int y = tm.tm_year;

	int build_num;

	build_num = (y - 1) * 365.25;
	build_num += d;
	if (y % 4 == 0 && m > 1)
		build_num += 1;
	build_num -= 35739;

	release.build = build_num;

	// Find download link
	for (auto asset : res["assets"])
	{
		if (asset["name"] == ASSET_NAME)
		{
			release.url = asset["browser_download_url"];
		}
	}

	if (release.url == "")
	{
		DevMsg("Cannot find download URL for " ASSET_NAME "\n");
		return false;
	}

	// Get release note
	release.body = res["body"];

	return true;
}

bool Updater::ReloadPlugin()
{
	if (!fs::exists(sptPath))
	{
		// Prevent renaming the plugin DLL file without replacing a file with the same name back.
		Warning("Failed to reload: plugin path does not exist.\n");
		return false;
	}

	auto& plugins = *(CUtlVector<void*>*)((uint32_t)interfaces::pluginHelpers + 4);
	extern CSourcePauseTool g_SourcePauseTool;
	for (int i = 0; i < plugins.Count(); i++)
	{
		// m_pPlugin
		if (*(IServerPluginCallbacks**)((uint32_t)plugins[i] + 132) == &g_SourcePauseTool)
		{
			std::string gamePath = GetGameDir();
			char cmd[512];
			sprintf(cmd,
			        "plugin_unload %d; plugin_load \"%s\";",
			        i,
			        fs::relative(sptPath, gamePath).string().c_str());
			EngineConCmd(cmd);
			return true;
		}
	}
	return false;
}

void Updater::UpdatePlugin()
{
	std::string tmpPath = std::filesystem::temp_directory_path().append(ASSET_NAME).string();

	Msg("Downloading " ASSET_NAME " from '%s'\n", release.url.c_str());
	if (!Download(release.url.c_str(), tmpPath.c_str()))
	{
		Msg("An error occurred.\n");
		return;
	}

	// In Windows, you can't delete loaded DLL file, but can rename it.
	std::filesystem::rename(sptPath, ASSET_NAME ".old-auto");

	std::filesystem::copy(tmpPath, sptPath);
	std::filesystem::remove(tmpPath);

	Msg(ASSET_NAME " successfully installed.\n");

#ifdef OE
	Msg("To complete the installation, please restart the game.\n");
#else
	Msg("Reloading plugin...\n");

	if (!ReloadPlugin())
	{
		Msg("An error occurred when reloading. To complete the installation, please restart the game.\n");
	}
#endif
}

int Updater::CheckUpdate()
{
	Msg("Fetching latest version information...\n");
	if (!FetchReleaseInfo())
	{
		Msg("An error occurred.\n");
		return UPDATER_ERROR;
	}

	int latestBuildNum = release.build;
	int currentBuildNum = GetPluginBuildNumber();

	DevMsg("Latest: %d\n", latestBuildNum);
	DevMsg("Current: %d\n", currentBuildNum);
	Msg("[Latest] %s\n", release.name.c_str());

	if (currentBuildNum == latestBuildNum)
		return UPDATER_UP_TO_DATE;

	if (currentBuildNum > latestBuildNum)
		return UPDATER_AHEAD;

	Msg("%s\n\n", release.body.c_str());
	return UPDATER_OUTDATED;
}

#endif
