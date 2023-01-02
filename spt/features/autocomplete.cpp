#include "stdafx.hpp"
#include "..\feature.hpp"
#include "..\utils\command.hpp"
#include "..\utils\game_detection.hpp"
#include "interfaces.hpp"

// Autocomplete for existing ConCommand
class AutocompleteFeature : public FeatureWrapper<AutocompleteFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	ConCommand_guts* playdemoCommand;
	FnCommandCompletionCallback ORIG_playdemoCompleteFunc;
	ConCommand_guts* loadCommand;
	FnCommandCompletionCallback ORIG_loadCompleteFunc;
	ConCommand_guts* execCommand;
	FnCommandCompletionCallback ORIG_execCompleteFunc;
};

static AutocompleteFeature spt_autocomplete;

DECLARE_AUTOCOMPLETIONFILE_FUNCTION(playdemo, "", ".dem");
DECLARE_AUTOCOMPLETIONFILE_FUNCTION(load, "save", ".sav");
DECLARE_AUTOCOMPLETIONFILE_FUNCTION(exec, "cfg", ".cfg");

bool AutocompleteFeature::ShouldLoadFeature()
{
	if (utils::DoesGameLookLikeDMoMM())
		return false;
	return !!(interfaces::g_pCVar);
}

void AutocompleteFeature::InitHooks() {}

void AutocompleteFeature::LoadFeature()
{
#ifndef OE
	playdemoCommand = reinterpret_cast<ConCommand_guts*>(interfaces::g_pCVar->FindCommand("playdemo"));
	loadCommand = reinterpret_cast<ConCommand_guts*>(interfaces::g_pCVar->FindCommand("load"));
	execCommand = reinterpret_cast<ConCommand_guts*>(interfaces::g_pCVar->FindCommand("exec"));
#else
	playdemoCommand = reinterpret_cast<ConCommand_guts*>(FindCommand("playdemo"));
	loadCommand = reinterpret_cast<ConCommand_guts*>(FindCommand("load"));
	execCommand = reinterpret_cast<ConCommand_guts*>(FindCommand("exec"));
#endif

	if (playdemoCommand)
	{
		ORIG_playdemoCompleteFunc = playdemoCommand->m_fnCompletionCallback;
		playdemoCommand->m_fnCompletionCallback = AUTOCOMPLETION_FUNCTION(playdemo);
	}

	if (loadCommand)
	{
		ORIG_loadCompleteFunc = loadCommand->m_fnCompletionCallback;
		loadCommand->m_fnCompletionCallback = AUTOCOMPLETION_FUNCTION(load);
	}

	if (execCommand)
	{
		ORIG_execCompleteFunc = playdemoCommand->m_fnCompletionCallback;
		execCommand->m_fnCompletionCallback = AUTOCOMPLETION_FUNCTION(exec);
	}
}

void AutocompleteFeature::UnloadFeature()
{
	if (playdemoCommand)
		playdemoCommand->m_fnCompletionCallback = ORIG_playdemoCompleteFunc;

	if (loadCommand)
		loadCommand->m_fnCompletionCallback = ORIG_loadCompleteFunc;

	if (execCommand)
		execCommand->m_fnCompletionCallback = ORIG_execCompleteFunc;
}
