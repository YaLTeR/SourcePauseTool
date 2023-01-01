#include "stdafx.h"
#include "..\feature.hpp"
#include "convar.hpp"
#include "game_detection.hpp"

static void NoclipNofixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue);

ConVar y_spt_noclip_nofix("y_spt_noclip_nofix",
						  "0",
						  0,
                          "Disables noclip's position fixing.",
                          NoclipNofixCVarCallback);

// Gives the option to disable all extra flashlight movement, including delay, swaying, and bobbing.
class NoclipNofixFeature : public FeatureWrapper<NoclipNofixFeature>
{
public:
	void Toggle(bool enabled);


protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	
	// value replacements and single instruction overrides
	// we'll just write the bytes ourselves...
private:
	uintptr_t ORIG_Server__NoclipString;
	std::vector<patterns::MatchedPattern> MATCHES_Server__StringReferences;

	DECL_BYTE_REPLACE(Jump, 6);

	byte inscCompare[4] = {0x84, 0xC0, 0x0F, 0x85};
	int jmpDistance = 0x0;
};

static NoclipNofixFeature spt_noclipnofix;
static void NoclipNofixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue)
{
	spt_noclipnofix.Toggle(((ConVar*)pConVar)->GetBool());
}

namespace patterns
{
	PATTERNS(Server__NoclipString, "1", "6E 6F 63 6C 69 70 20 4F 46 46 0A 00");
	PATTERNS(Server__StringReferences, "1", "68");
}

bool NoclipNofixFeature::ShouldLoadFeature()
{
	return true;
}

void NoclipNofixFeature::InitHooks()
{
	FIND_PATTERN(server, Server__NoclipString);
	FIND_PATTERN_ALL(server, Server__StringReferences);
}

void NoclipNofixFeature::LoadFeature() 
{
	if (!ORIG_Server__NoclipString || MATCHES_Server__StringReferences.empty())
		return;

	for (auto match : MATCHES_Server__StringReferences)
	{
		if (*(uintptr_t*)(match.ptr + 1) != ORIG_Server__NoclipString)
			continue;

		/*
		* After printing "noclip OFF\n", the game will do "if ( !TestEntityPosition( pPlayer ) )",
		* which generally translates to
		*	xx					PUSH pPlayer
		*	E8 xx xx xx xx		CALL [TestEntityPosition]
		*	~ noise ~
		*	84 c0				TEST al, al
		*	0f 85 xx xx 00 00	JNE [eof] <-- we'll asume its less than 0x10000 bytes away...
		*/
		byte* bytes = (byte*)match.ptr;
		for (int i = 0; i <= 0x200; i++)
		{
			int jmpOff = *(int*)(bytes + i + 4);
			if (jmpOff > 0x10000)
				continue;

			if (memcmp((void*)(bytes + i), (void*)inscCompare, 4))
				continue;

			INIT_BYTE_REPLACE(Jump, (uintptr_t)(bytes + i + 2));

			/*
			* When we replace the instruction with a JMP, we replace the last byte with NOP,
			* meaning the starting point of our jump is shifted up by 1 to that byte.
			*/
			jmpDistance = jmpOff + 1;
			goto success;
		}

	}
	return;
	
	success:;
	InitConcommandBase(y_spt_noclip_nofix);
}

void NoclipNofixFeature::UnloadFeature()
{
	DESTROY_BYTE_REPLACE(Jump);
}


void NoclipNofixFeature::Toggle(bool enabled)
{
	if (PTR_Jump == 0)
		return;

	if (enabled)
	{
		MemUtils::ReplaceBytes((void*)PTR_Jump, 1, new uint8[1]{0xE9});
		MemUtils::ReplaceBytes((void*)(PTR_Jump + 1), 4, (uint8*)&jmpDistance);
		MemUtils::ReplaceBytes((void*)(PTR_Jump + 5), 1, new uint8[1]{0x90});
	}
	else
	{
		RESTORE_BYTE_REPLACE(Jump);
	}
}
