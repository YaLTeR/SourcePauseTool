#ifndef OE
#include "..\feature.hpp"

typedef void(
    __fastcall* _CViewRender__RenderView)(void* thisptr, int edx, void* cameraView, int nClearFlags, int whatToDraw);
typedef void(__fastcall* _CViewRender__Render)(void* thisptr, int edx, void* rect);

// Overlay hook stuff, could combine with overlay renderer as well
class Overlay : public Feature
{
public:
	bool renderingOverlay = false;
	void* screenRect = nullptr;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	_CViewRender__RenderView ORIG_CViewRender__RenderView = nullptr;
	_CViewRender__Render ORIG_CViewRender__Render = nullptr;

	static void __fastcall HOOKED_CViewRender__RenderView(void* thisptr,
	                                                      int edx,
	                                                      void* cameraView,
	                                                      int nClearFlags,
	                                                      int whatToDraw);
	static void __fastcall HOOKED_CViewRender__Render(void* thisptr, int edx, void* rect);
};

extern Overlay spt_overlay;
#endif