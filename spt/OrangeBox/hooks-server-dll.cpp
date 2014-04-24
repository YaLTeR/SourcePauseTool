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

            ptrdiff_t off1M_nOldButtons;
            ptrdiff_t off2M_nOldButtons;
            bool cantJumpNextTime;
        } hookState;

        namespace Internal
        {
            bool __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx )
            {
                /*
                    CheckJumpButton calls FinishGravity() if and only if we jumped.
                */
                const int IN_JUMP = (1 << 1);

                int *pM_nOldButtons = NULL;
                int origM_nOldButtons = 0;

                if ( y_spt_autojump.GetBool() )
                {
                    pM_nOldButtons = (int *)(*((DWORD_PTR *)thisptr + hookState.off1M_nOldButtons) + hookState.off2M_nOldButtons);
                    origM_nOldButtons = *pM_nOldButtons;

                    if ( !hookState.cantJumpNextTime ) // Do not do anything if we jumped on the previous tick.
                    {
                        *pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
                    }
                    else
                    {
                        EngineDevLog( "SPT: Con jump prevented!\n" );
                    }
                }

                hookState.cantJumpNextTime = false;

                bool rv = hookState.ORIG_CheckJumpButton( thisptr, edx ); // This function can only change the jump bit.

                if ( y_spt_autojump.GetBool() )
                {
                    if ( !(*pM_nOldButtons & IN_JUMP) ) // CheckJumpButton didn't change anything (we didn't jump).
                    {
                        *pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
                    }
                }

                if ( rv )
                {
                    // We jumped.
                    if ( y_spt_autojump_ensure_legit.GetBool() )
                    {
                        hookState.cantJumpNextTime = true; // Prevent consecutive jumps.
                    }

                    EngineDevLog( "SPT: Jump!\n" );
                }

                // EngineDevLog( "SPT: Engine call: [server dll] CheckJumpButton() => %s\n", (rv ? "true" : "false") );

                return rv;
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

                switch (ptnNumber)
                {
                case 0:
                    hookState.off1M_nOldButtons = 2;
                    hookState.off2M_nOldButtons = 40;
                    break;
                case 1:
                    hookState.off1M_nOldButtons = 1;
                    hookState.off2M_nOldButtons = 40;
                    break;
                }
            }
            else
            {
                EngineWarning( "SPT: [server dll] Could not find CheckJumpButton!\n" );
                EngineWarning( "SPT: [server dll] y_spt_autojump has no effect.\n" );
            }

            AttachDetours( moduleName, 2,
                &hookState.ORIG_CheckJumpButton, Internal::HOOKED_CheckJumpButton );
        }

        void Unhook( std::wstring &moduleName )
        {
            DetachDetours( moduleName, 2,
                &hookState.ORIG_CheckJumpButton, Internal::HOOKED_CheckJumpButton );

            Clear();
        }

        void Clear()
        {
            hookState.moduleInfo.hModule = NULL;
            hookState.moduleInfo.moduleStart = NULL;
            hookState.moduleInfo.moduleLength = NULL;
            hookState.ORIG_CheckJumpButton = NULL;
            hookState.off1M_nOldButtons = 0;
            hookState.off2M_nOldButtons = 0;
            hookState.cantJumpNextTime = false;
        }
    }
}