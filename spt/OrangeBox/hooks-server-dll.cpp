#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>

#include "../hooks.hpp"
#include "../detoursutils.hpp"
#include "../memutils.hpp"
#include "../patterns.hpp"
#include "../spt.hpp"
#include "cvars.hpp"

namespace Hooks
{
    namespace ServerDll
    {
        struct
        {
            module_info_t moduleInfo;

            _CheckJumpButton ORIG_CheckJumpButton;
            _FinishGravity ORIG_FinishGravity;

            bool inCheckJumpButton;
            bool cantJumpNextTime;
        } hookState;

        namespace Internal
        {
            void __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx )
            {
                // EngineDevLog( "SPT: Engine call: [server dll] CheckJumpButton!\n" );

                /*
                    CheckJumpButton calls FinishGravity() if and only if we jumped.
                */
                const int IN_JUMP = (1 << 1);

                int *pM_nOldButtons = NULL;
                int origM_nOldButtons = 0;

                if ( y_spt_autojump.GetBool() )
                {
                    pM_nOldButtons = (int *)(*((DWORD_PTR *)thisptr + 2) + 40);
                    origM_nOldButtons = *pM_nOldButtons;

                    if ( !hookState.cantJumpNextTime ) // Do not do anything if we jumped on the previous tick.
                    {
                        *pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
                    }
                }

                hookState.cantJumpNextTime = false;

                hookState.inCheckJumpButton = true;
                hookState.ORIG_CheckJumpButton( thisptr, edx ); // This function can only change the jump bit.
                hookState.inCheckJumpButton = false;

                if ( y_spt_autojump.GetBool() )
                {
                    if ( !(*pM_nOldButtons & IN_JUMP) ) // CheckJumpButton didn't change anything (we didn't jump).
                    {
                        *pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
                    }
                }
            }

            void __fastcall HOOKED_FinishGravity( void *thisptr, int edx )
            {
                if (hookState.inCheckJumpButton)
                {
                    // We jumped.
                    hookState.cantJumpNextTime = true; // Prevent consecutive jumps.

                    // EngineDevLog( "SPT: Jump!\n" );
                }

                return hookState.ORIG_FinishGravity( thisptr, edx );
            }
        }

        void Hook( std::wstring &moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength )
        {
            Clear(); // Just in case.

            hookState.moduleInfo.hModule = hModule;
            hookState.moduleInfo.moduleStart = moduleStart;
            hookState.moduleInfo.moduleLength = moduleLength;

            MemUtils::ptnvec_size ptnNumber;

            // CheckJumpButton
            EngineLog( "SPT: [server dll] Searching for CheckJumpButton...\n" );

            DWORD_PTR pCheckJumpButton = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsServerCheckJumpButton, &pCheckJumpButton );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
                EngineLog( "SPT: [server dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsServerCheckJumpButton[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "SPT: [server dll] Could not find CheckJumpButton!\n" );
                EngineWarning( "SPT: [server dll] y_spt_autojump has no effect.\n" );
            }

            // FinishGravity
            EngineLog( "SPT: [server dll] Searching for FinishGravity...\n" );

            DWORD_PTR pFinishGravity = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsServerFinishGravity, &pFinishGravity );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_FinishGravity = (_FinishGravity)pFinishGravity;
                EngineLog( "SPT: [server dll] Found FinishGravity at %p (using the build %s pattern).\n", pFinishGravity, Patterns::ptnsServerFinishGravity[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "SPT: [server dll] Could not find FinishGravity!\n" );
                EngineWarning( "SPT: [server dll] Consecutive jump protection is disabled.\n" );
            }

            AttachDetours( moduleName, 4,
                &hookState.ORIG_CheckJumpButton, Internal::HOOKED_CheckJumpButton,
                &hookState.ORIG_FinishGravity, Internal::HOOKED_FinishGravity );
        }

        void Unhook( std::wstring &moduleName )
        {
            DetachDetours( moduleName, 4,
                &hookState.ORIG_CheckJumpButton, Internal::HOOKED_CheckJumpButton,
                &hookState.ORIG_FinishGravity, Internal::HOOKED_FinishGravity );

            Clear();
        }

        void Clear()
        {
            hookState.moduleInfo.hModule = NULL;
            hookState.moduleInfo.moduleStart = NULL;
            hookState.moduleInfo.moduleLength = NULL;
            hookState.ORIG_CheckJumpButton = NULL;
            hookState.ORIG_FinishGravity = NULL;
            hookState.inCheckJumpButton = false;
            hookState.cantJumpNextTime = false;
        }
    }
}