#pragma once
#if defined(SSDK2007) || defined(SSDK2013)
#include "..\feature.hpp"
#include "icliententity.h"

extern IClientEntity* getPortal(const char* arg, bool verbose);
extern IClientEntity* GetLinkedPortal(IClientEntity* portal);

// VAG tester
class VagSearcher : public FeatureWrapper<VagSearcher>
{
public:
	void StartSearch();
	void VagCrashTriggered();
	bool IsIterating()
	{
		return iteration != 0;
	}
	enum VagSearchResult
	{
		ITERATING,
		SUCCESS,
		FAIL,
		MAX_ITERATIONS,
		WOULD_CAUSE_CRASH
	};
	enum SearchResult
	{
		NONE,
		NEXT_TO_ENTRY,
		NEXT_TO_EXIT,
		BEHIND_ENTRY_PLANE
	};
	void StartIterations();
	VagSearchResult RunIteration();
	void OnTick();

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	int cooldown_ticks = 2;
	int cooldown;
	int max_iteration = 35;
	int iteration = 0;
	bool crash;
	IClientEntity* enter_portal = NULL;
	IClientEntity* exit_portal = NULL;
	int entry_index;
	int exit_index;
	Vector entry_origin;
	Vector entry_norm;
	Vector exit_origin;
	bool is_crouched;
	int player_half_height;
	Vector player_setpos;
	int no_idx;
	SearchResult first_result = NONE;
	SearchResult current_result;
	std::string setpos_cmd;
};

extern VagSearcher spt_vag_searcher;

#endif