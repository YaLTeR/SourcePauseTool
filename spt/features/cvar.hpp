#pragma once

#include "spt/feature.hpp"

class CvarStuff : public FeatureWrapper<CvarStuff>
{
public:
	void UpdateCommandList();
	std::vector<std::string> dev_cvars;
#ifdef OE
	std::vector<ConCommandBase*> hidden_cvars;
#endif

protected:
	virtual void LoadFeature() override;
#ifdef OE
	virtual void InitHooks() override;
	virtual void PreHook() override;

private:
	DECL_HOOK_THISCALL(void, RebuildCompletionList, void*, const char* text);
	struct CompletionItem
	{
		bool m_bIsCommand;
		ConCommandBase* m_pCommand;
		void* m_pText; // CHistoryItem*
	};
	int m_CompletionListOffset = 0;
#endif
};

extern CvarStuff spt_cvarstuff;
