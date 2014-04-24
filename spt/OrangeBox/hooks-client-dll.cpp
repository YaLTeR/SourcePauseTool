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

            void __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx )
            {
                EngineDevLog( "SPT: Engine call: [client dll] CheckJumpButton!\n" );

                return hookState.ORIG_CheckJumpButton( thisptr, edx );
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
            EngineLog( "SPT: Searching for DoImageSpaceMotionBlur...\n" );

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
            EngineLog( "SPT: [client dll] Searching for CheckJumpButton...\n" );

            DWORD_PTR pCheckJumpButton = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsCheckJumpButton, &pCheckJumpButton );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
                EngineLog( "SPT: [client dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsCheckJumpButton[ptnNumber].build.c_str() );

                /*switch (ptnNumber)
                {
                case 0:
                hookState.pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 132);
                break;
                }

                Log( "SPT: pgpGlobals is %p.\n", hookState.pgpGlobals );*/
            }
            else
            {
                EngineWarning( "SPT: [client dll] Could not find CheckJumpButton!\n" );
                EngineWarning( "SPT: [client dll] y_spt_autojump has no effect.\n" );
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
            hookState.pgpGlobals = NULL;
        }
    }
}