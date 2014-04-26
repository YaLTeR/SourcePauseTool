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
    namespace ClientDll
    {
        struct
        {
            module_info_t moduleInfo;

            _DoImageSpaceMotionBlur ORIG_DoImageSpaceMorionBlur;
            _CheckJumpButton ORIG_CheckJumpButton;

            DWORD_PTR *pgpGlobals;
            ptrdiff_t off1M_nOldButtons;
            ptrdiff_t off2M_nOldButtons;
            bool cantJumpNextTime;
        } hookState;

        namespace Internal
        {
            void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h )
            {
                DWORD_PTR origgpGlobals = NULL;

                /*
                Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
                so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
                function does not use anything apart from gpGlobals->realtime from gpGlobals,
                so we can do such a replace to make it use gpGlobals->curtime instead without
                breaking anything else.
                */
                if (hookState.pgpGlobals)
                {
                    if (y_spt_motion_blur_fix.GetBool())
                    {
                        origgpGlobals = *hookState.pgpGlobals;
                        *hookState.pgpGlobals = *hookState.pgpGlobals + 12;
                    }
                }

                hookState.ORIG_DoImageSpaceMorionBlur( view, x, y, w, h );

                if (hookState.pgpGlobals)
                {
                    if (y_spt_motion_blur_fix.GetBool())
                    {
                        *hookState.pgpGlobals = origgpGlobals;
                    }
                }
            }

            bool __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx )
            {
                /*
                    Not sure if this gets called at all from the client dll, but
                    I will just hook it in exactly the same way as the server one.

                    CheckJumpButton calls FinishGravity() if and only if we jumped.
                */
                const int IN_JUMP = (1 << 1);

                int *pM_nOldButtons = NULL;
                int origM_nOldButtons = 0;

                if (y_spt_autojump.GetBool())
                {
                    pM_nOldButtons = (int *)(*((DWORD_PTR *)thisptr + hookState.off1M_nOldButtons) + hookState.off2M_nOldButtons);
                    origM_nOldButtons = *pM_nOldButtons;

                    if (!hookState.cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
                    {
                        *pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
                    }
                    else
                    {
                        // EngineDevLog( "SPT: Con jump prevented!\n" );
                    }
                }

                hookState.cantJumpNextTime = false;

                bool rv = hookState.ORIG_CheckJumpButton( thisptr, edx ); // This function can only change the jump bit.

                if (y_spt_autojump.GetBool())
                {
                    if ( !(*pM_nOldButtons & IN_JUMP) ) // CheckJumpButton didn't change anything (we didn't jump).
                    {
                        *pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
                    }
                }

                if (rv)
                {
                    // We jumped.
                    if (y_spt_autojump_ensure_legit.GetBool())
                    {
                        hookState.cantJumpNextTime = true; // Prevent consecutive jumps.
                    }

                    // EngineDevLog( "SPT: Jump!\n" );
                }

                EngineDevLog( "SPT: Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false") );

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

            // DoImageSpaceMotionBlur
            EngineDevLog( "SPT: Searching for DoImageSpaceMotionBlur...\n" );

            DWORD_PTR pDoImageSpaceMotionBlur = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsDoImageSpaceMotionBlur, &pDoImageSpaceMotionBlur );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_DoImageSpaceMorionBlur = (_DoImageSpaceMotionBlur)pDoImageSpaceMotionBlur;
                EngineLog( "SPT: Found DoImageSpaceMotionBlur at %p (using the build %s pattern).\n", pDoImageSpaceMotionBlur, Patterns::ptnsDoImageSpaceMotionBlur[ptnNumber].build.c_str() );

                switch (ptnNumber)
                {
                case 0:
                    hookState.pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 132);
                    break;

                case 1:
                    hookState.pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 153);
                    break;

                case 2:
                    hookState.pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 129);
                    break;
                }

                EngineLog( "SPT: pgpGlobals is %p.\n", hookState.pgpGlobals );
            }
            else
            {
                EngineWarning( "SPT: Could not find DoImageSpaceMotionBlur!\n" );
                EngineWarning( "SPT: y_spt_motion_blur_fix has no effect.\n" );
            }

            // CheckJumpButton
            EngineDevLog( "SPT: [client dll] Searching for CheckJumpButton...\n" );

            DWORD_PTR pCheckJumpButton = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsClientCheckJumpButton, &pCheckJumpButton );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
                EngineLog( "SPT: [client dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsClientCheckJumpButton[ptnNumber].build.c_str() );

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

                case 2:
                    hookState.off1M_nOldButtons = 2;
                    hookState.off2M_nOldButtons = 40;
                    break;
                }
            }
            else
            {
                EngineWarning( "SPT: [client dll] Could not find CheckJumpButton.\n" );
                EngineWarning( "SPT: [client dll] This should not matter.\n" );
            }

            AttachDetours( moduleName, 4,
                &hookState.ORIG_DoImageSpaceMorionBlur, Internal::HOOKED_DoImageSpaceMotionBlur,
                &hookState.ORIG_CheckJumpButton, Internal::HOOKED_CheckJumpButton );
        }

        void Unhook( std::wstring &moduleName )
        {
            DetachDetours( moduleName, 4,
                &hookState.ORIG_DoImageSpaceMorionBlur, Internal::HOOKED_DoImageSpaceMotionBlur,
                &hookState.ORIG_CheckJumpButton, Internal::HOOKED_CheckJumpButton );

            Clear();
        }

        void Clear()
        {
            hookState.moduleInfo.hModule = NULL;
            hookState.moduleInfo.moduleStart = NULL;
            hookState.moduleInfo.moduleLength = NULL;
            hookState.ORIG_DoImageSpaceMorionBlur = NULL;
            hookState.ORIG_CheckJumpButton = NULL;
            hookState.pgpGlobals = NULL;
            hookState.off1M_nOldButtons = 0;
            hookState.off2M_nOldButtons = 0;
            hookState.cantJumpNextTime = false;
        }
    }
}
