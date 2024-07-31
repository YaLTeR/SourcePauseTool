#include "stdafx.hpp"
#include "..\feature.hpp"
#include "thirdparty/imgui/imgui.h"
#include "interfaces.hpp"

#include "thirdparty/x86.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_impl_dx9.h"
#include "thirdparty/imgui/imgui_impl_win32.h"

#include "d3d9helper.h"

class ImGuiImpl : public FeatureWrapper<ImGuiImpl>
{
private:
	HWND hWnd;
	inline static WNDPROC g_originalWndProc = nullptr;

	enum class LoadState
	{
		None,
		CreatedContext,
		InitWin32,
		InitDx9,
		OverrideWndProc,

		Loaded = OverrideWndProc,
	} loadState;

	bool PtrInModule(void* ptr, size_t nBytes, void* modBase, size_t modSize)
	{
		return ptr >= modBase && (char*)ptr + nBytes <= (char*)modBase + modSize
		       && (char*)ptr + nBytes >= modBase;
	}

	static LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
		                                                             UINT msg,
		                                                             WPARAM wParam,
		                                                             LPARAM lParam);
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;
		return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
	}

protected:
	virtual void InitHooks()
	{
		/*
		* We need:
		* - a hook for CShaderDeviceDx8::Present()
		* - the IDirect3DDevice9 to give to ImGui
		* 
		* Conveniently, the Present() function references the device, so we first find the
		* Present() function in the device wrapper that the SDK exposes, then look for the
		* device in it.
		*/

		void* moduleBase;
		size_t moduleSize;
		MemUtils::GetModuleInfo(L"shaderapidx9.dll", nullptr, &moduleBase, &moduleSize);

		const int maxInstrSearch = 64;
		const int maxVtableSearch = 16;
		IShaderDevice* deviceWrapper = interfaces::shaderDevice;
		if (!PtrInModule(deviceWrapper, maxVtableSearch * sizeof(void*), moduleBase, moduleSize))
			return;

		IDirect3DDevice9* device = nullptr;
		int vOff = -1;
		while (!device && ++vOff < maxVtableSearch)
		{
			void** pPresentFunc = ((void***)deviceWrapper)[0] + vOff;
			if (!PtrInModule(pPresentFunc, maxInstrSearch, moduleBase, moduleSize))
				continue;

			/*
			* CShaderDeviceDx8::Present() does two virtual calls prior to calling Dx9Device().
			* Dx9Device() may or may not be inlined, so follow a single call if necessary. Once
			* we see the two virtual calls, we expect something of the form `MOV REG, ADDR` to
			* tell us the address of the IDirect3DDevice9.
			*/

			uint8_t* func = (uint8_t*)*pPresentFunc;
			int len = 0;
			int nVirtualCalls = 0;

			for (uint8_t* addr = func; len != -1 && addr - func < maxInstrSearch && nVirtualCalls <= 2;
			     len = x86_len(addr), addr += len)
			{
				if (addr[0] == X86_MISCMW && (addr[1] & 0b10111000) == 0b10010000)
					nVirtualCalls++; // CALL REG | CALL [REG + OFF]
				else if (nVirtualCalls == 2 && addr[0] == X86_CALL)
					func = addr = addr + 5 + *(uint32_t*)(addr + 1); // CALL OFF
				else if (addr[0] == X86_INT3)
					break;
				if (nVirtualCalls == 2
				    && ((addr[0] & 0b11111000) == 0b10111000 || (addr[0] & 0b11111000) == 0b10100000))
				{
					// MOV REG ADDR | MOV REG [ADDR]
					IDirect3DDevice9** ppDevice = *(IDirect3DDevice9***)(addr + 1);
					if (!PtrInModule(ppDevice, sizeof *ppDevice, moduleBase, moduleSize))
						continue;
					device = *ppDevice;
					break;
				}
			}
		}
		if (!device)
			return;

		D3DDEVICE_CREATION_PARAMETERS params;
		if (device->GetCreationParameters(&params) != D3D_OK)
			return;
		hWnd = params.hFocusWindow;

		AddVFTableHook(
		    VFTableHook{
		        *(void***)deviceWrapper,
		        vOff,
		        (void*)HOOKED_CShaderDeviceDx8__Present,
		        (void**)&ORIG_CShaderDeviceDx8__Present,
		    },
		    "shaderapidx9");

		if (!IMGUI_CHECKVERSION())
			return;
		if (!ImGui::CreateContext())
			return;
		loadState = LoadState::CreatedContext;
		if (!ImGui_ImplWin32_Init(hWnd))
			return;
		loadState = LoadState::InitWin32;
		if (!ImGui_ImplDX9_Init(device))
			return;
		loadState = LoadState::InitDx9;
		Assert(!g_originalWndProc);
		g_originalWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
		if (!g_originalWndProc)
			return;
		loadState = LoadState::OverrideWndProc;
		DevMsg("Initialized Dear ImGui.\n");
	};

	virtual void UnloadFeature()
	{
#pragma warning(push)
#pragma warning(disable : 26819)
		switch (loadState)
		{
		case LoadState::OverrideWndProc:
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
		case LoadState::InitDx9:
			ImGui_ImplDX9_Shutdown();
		case LoadState::InitWin32:
			ImGui_ImplWin32_Shutdown();
		case LoadState::CreatedContext:
			ImGui::DestroyContext();
		case LoadState::None:
		default:
			g_originalWndProc = nullptr;
			hWnd = nullptr;
			loadState = LoadState::None;
			break;
		}
#pragma warning(pop)
	};

	DECL_HOOK_THISCALL(void, CShaderDeviceDx8__Present, void*);
};

static ImGuiImpl spt_imgui;

IMPL_HOOK_THISCALL(ImGuiImpl, void, CShaderDeviceDx8__Present, void*)
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	spt_imgui.ORIG_CShaderDeviceDx8__Present(thisptr);
}
