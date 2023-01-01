#include "stdafx.h"
#include "..\feature.hpp"
#include "convar.hpp"
#include "autojump.hpp"
#include "interfaces.hpp"

static void FreeOOBCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue);

ConVar y_spt_free_oob_movement("y_spt_free_oob_movement",
						      "0",
						       0,
                              "Disables noclip's position fixing.",
                               FreeOOBCVarCallback);

// Gives the option to disable all extra flashlight movement, including delay, swaying, and bobbing.
class FreeOobFeature : public FeatureWrapper<FreeOobFeature>
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
	DECL_BYTE_REPLACE(FirstJump, 6, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90);
	DECL_BYTE_REPLACE(SecondJump, 6, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90);

	uint8 firstJumpCompare[2] = {0x0F, 0x85};
	uint8 secondJumpCompare[5] = {0xF6, 0xC4, 0x44, 0x0F, 0x8A};
	uintptr_t ptrTryPlayerMove = 0;

	uintptr_t ORIG_CheckJumpButton;
};

static FreeOobFeature spt_freeoob;
static void FreeOOBCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue)
{
	spt_freeoob.Toggle(((ConVar*)pConVar)->GetBool());
}


bool FreeOobFeature::ShouldLoadFeature()
{
	return true;
}

void FreeOobFeature::InitHooks()
{
	FIND_PATTERN(server, CheckJumpButton);
}

void FreeOobFeature::LoadFeature()
{
	if (!ORIG_CheckJumpButton || !interfaces::gm)
		return;

	uintptr_t vftEntry = *(uintptr_t*)interfaces::gm;
	if (vftEntry == NULL)
		return;

	GET_MODULE(server);

	// we assume TryPlayerMove is 2 entries below CheckJumpButton in the vftable.
	uintptr_t cjbPtr = (uintptr_t)ORIG_CheckJumpButton;
	for (int i = 0; i <= 0x100; i++)
	{
		auto funcPtr = *(uintptr_t*)(vftEntry + i * 4);
		if (funcPtr != cjbPtr)
			continue;

		/*
		* velocity is deleted once "if ( stuck.startsolid || stuck.fraction != 1.0f )" passes
		* when compiled, this condition is flipped.
		* 
		* we can more reliably find the 2nd condiition, so we'll do such, then look back a
		* limited number of bytes until we find a jump to the same location.
		* 
		* 0f 85 ?? ?? 00 00			JNE [location]
		* ~ noise ~
		* f6 c4 44					TEST AH,0x44
		* 0f 8a ?? ?? 00 00			JP [location]
		* 
		* we will assume both offsets in the jumps are under 0x10000
		*/
		
		auto funcStart = *(uintptr_t*)(vftEntry + (i + 3) * 4);
		for (int j = 0; j <= 0x500; j++)
		{
			uintptr_t cur = funcStart + j;
			if (memcmp((void*)cur, (void*)secondJumpCompare, 5))
				continue;

			int off1 = *(int*)(cur + 3 + 2);
			if (off1 < 0 || off1 >= 0x10000)
				continue;

			uintptr_t location = cur + 3 + 6 + off1;
			PTR_FirstJump = cur + 3;

			for (int k = 0; k <= 0x50; k++)
			{
				cur = funcStart + j - k;
				if (memcmp((void*)cur, (void*)firstJumpCompare, 2))
					continue;

				int off2 = *(int*)(cur + 2);
				if (off2 < 0 || off2 >= 0x10000)
					continue;

				if (cur + 6 + off2 != location)
					continue;

				INIT_BYTE_REPLACE(FirstJump, PTR_FirstJump);
				INIT_BYTE_REPLACE(SecondJump, cur);

				goto success;
			}

			return;
		}
	}

	return;

	success:
	InitConcommandBase(y_spt_free_oob_movement);
}

void FreeOobFeature::UnloadFeature()
{
	DESTROY_BYTE_REPLACE(FirstJump);
	DESTROY_BYTE_REPLACE(SecondJump);
}


void FreeOobFeature::Toggle(bool enabled)
{
	if (enabled)
	{
		DO_BYTE_REPLACE(FirstJump);
		DO_BYTE_REPLACE(SecondJump);
	}
	else
	{
		RESTORE_BYTE_REPLACE(FirstJump);
		RESTORE_BYTE_REPLACE(SecondJump);
	}
}
