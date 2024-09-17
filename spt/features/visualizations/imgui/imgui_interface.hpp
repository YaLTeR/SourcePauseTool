#pragma once

#include "thirdparty/imgui/imgui.h"
#include "spt/cvars.hpp"

class SptImGuiFeature;
class SptImGui;
struct ImGuiWindow;

#define SPT_IMGUI_WARN_COLOR_YELLOW (ImVec4{.8f, .5f, .1f, 1})
#define SPT_IMGUI_WARN_COLOR_RED (ImVec4{.9f, .05f, .05f, 1})

/*
* This header is for registering features to display stuff with Dear ImGui. Some
* features are nice to have a GUI for, some might only be nice to use through
* the console, and some could be a hybrid mix of both. Usually, I think it would
* be ideal for most features to be entirely usable through the console, and
* optionally have a GUI for *most* of its functionality. This is to allow users
* to set up binds for their favorite features! If a feature has some parts of it
* available through the console AND the GUI, please let the user know somehow
* (e.g. if you can change the value of a cvar with a checkbox or a text field in
* ImGui, show the specific cvar that is being set in the GUI with at least a
* tooltip or something).
* 
* To register a feature to draw something with Dear ImGui, use one of the 'register'
* functions below. To simplify the look of the GUI, I think that most of the
* functionality of SPT's features should fit within a single main window. I've tried
* to roughly organize SPT's features into categories and subcategories corresponding
* to tabs and subtabs in what I'm calling the "main" window. UI design is one of my
* "top 10 things I probably don't want to do for a living", so there might be a
* better way of organizing everything. This approach is pretty simple and scalable.
* 
* Each feature can:
* - live in a single tab/subtab in the main window
* - live in a single section within a tab
* - live in a separate window
* - a combination of the above - features are allowed to live across several
*   tabs in the main window, but that requires the use of several callbacks
* 
* Some important things to keep in mind:
* 
* - Don't reinterpret cast from color32 to ImU32! Use the conversion functions
*   below or better yet, use ImVec4 directly.
* 
* - Please save yourself some trouble and read about the ImGui stack system:
*   https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-about-the-id-stack-system
*   Go to DEV -> "Show ImGui example window", then Tools -> ID Stack Tool. This
*   is extremely helpful for determining why widgets don't react to input
*   events sometimes. All tabs/sections have their own ID scope.
*
* - Read about other debug tools: https://github.com/ocornut/imgui/wiki/Debug-Tools.
* 
* - The ImGui example window is your best friend for learning how to write your
*   own ImGui code. See: SPT -> DEV -> "Show ImGui example window". Look
*   through it to see if the thing you're trying to do is already in there,
*   then look for the code in imgui_demo.cpp.
* 
* - For displaying ConVars, I think a good approach is to let any value set
*   through the console take priority over any value set through the GUI.
*   Widgets like e.g. DragFloat() work nicely with this because they won't
*   override the value that you passed in unless the user starts dragging the
*   widget. For example if you want a listbox with 4 choices from a cvar, a
*   good approach would be to use var.GetInt() and clamp the value to [0, 3],
*   then only update the cvar if the user selects something in the widget.
* 
* - Just because a tab/section callback is called with open=false, that does
*   not necessarily mean that it is not open. It may mean that the tab is off
*   the screen and is clipped by ImGui. Let me know if you need to
*   differenciate between open and clipped.
* 
* - Callbacks will not be triggered one-to-one with e.g. SV_FrameSignal, some
*   frames may be dropped (e.g. dx9 device was lost or feature is disabled).
*   Don't use the imgui callbacks to update feature internals if you want to
*   e.g. check for something regularly.
* 
* - With the currently implementation of input events, ImGui will not see key
*   & mouse events when there is no game UI open.
*/

using SptImGuiTabCallback = std::function<void(bool open)>;
using SptImGuiSectionCallback = SptImGuiTabCallback;
using SptImGuiWindowCallback = std::function<void()>;
using SptImGuiHudTextCallback = std::function<void(ConVar& var)>;

/*
* This describes all tabs/sections in the main ImGui window. This approach of listing
* all the tabs in one place may seem a bit weird at first, but it makes it pretty
* easy to see the general UI layout at a glance (unlike spt_hud's approach of using a
* sort key). If you want to add your own section/tab, add it below! Do not make your
* own instances of the Section/Tab objects - the constructors here rely on static
* initialization order to determine the order of the tabs/subtabs/sections. The root
* tab should be first, and the dummy tab should be last. Try to order everything by
* most commonly used features first.
* 
* If a section/tab does not have a user defined callback, it will not be rendered!
* This applies recursively (if a tab has many child tabs/sections, and none of those
* children have callbacks, the parent tab won't be rendered). This is to prevent a
* bunch of empty sections/tabs from showing up when features fail to load. It also
* makes it really easy to see at a glance which features are supported in the current
* SPT/game version.
* 
* On the contrary, if you've registered a callback, it will always be rendered even
* if you don't draw anything. This means you should always draw something! If your
* feature is disabled because of game/version incompatibilities, don't register the
* callback. If your feature is disabled because of some other runtime condition
* (e.g. player has no portal gun or server is not active), draw the full UI of the
* feature within an ImGui::BeginDisabled block (and preferably let the user know why
* the feature is disabled in the UI).
*/
namespace SptImGuiGroup
{
	class Tab
	{
	private:
		friend class Section;
		friend class SptImGuiFeature;
		friend class SptImGui;
		const char* name;
		Tab* parent;
		SptImGuiTabCallback cb;
		bool hasAnyChildUserCallbacks = false;
		bool allChildrenRegistered = false;
		std::vector<class Section*> childSections;
		std::vector<Tab*> childTabs;

	public:
		Tab() = delete;
		Tab(Tab&) = delete;
		Tab(const char* name, Tab* parent);

	private:
		int CountLeafNodes() const;
		void SetupRenderCallbackRecursive();
		void RenderCallback(bool open) const;
		bool SetUserCallback(const SptImGuiTabCallback& cb);
		void ClearCallbacksRecursive();
	};

	// a section is a part of a tab used for simple features separated with ImGui::SeparatorText()
	class Section
	{
	private:
		friend class Tab;
		friend class SptImGuiFeature;
		friend class SptImGui;
		SptImGuiSectionCallback cb;
		const char* name;
		Tab* parent;

	public:
		Section() = delete;
		Section(Section&) = delete;
		Section(const char* name, Tab* parent);

	private:
		bool SetUserCallback(const SptImGuiTabCallback& cb);
	};

	// dummy tab, should be first - has the callback for rendering the window
	inline Tab Root{"SPT", nullptr};

	// render overlay
	inline Tab Overlay{"Overlay", &Root};

	// drawing visualizations
	inline Tab Draw{"Drawing", &Root};
	inline Tab Draw_Collides{"Collision", &Draw};
	inline Section Draw_Collides_World{"World Collides", &Draw_Collides};
	inline Section Draw_Collides_Ents{"Entity Collides", &Draw_Collides};
	inline Tab Draw_MapOverlay{"Map overlay", &Draw};
	inline Tab Draw_Lines{"Draw lines", &Draw};
	inline Tab Draw_PpPlacement{"Portal placement", &Draw};
	inline Section Draw_PpPlacement_Gun{"Gun portal placement", &Draw_PpPlacement};
	inline Section Draw_PpPlacement_Grid{"Portal placement grid", &Draw_PpPlacement};

	// features that access game state
	inline Tab GameIo{"Game IO", &Root};
	inline Tab GameIo_ISG{"ISG", &GameIo};

	// hud cvars - use the RegisterHudCvarXXX functions below to add cvars
	inline Tab Hud{"Text HUD", &Root};

	// development/debugging features
	inline Tab Dev{"DEV", &Root};
	inline Section Dev_GameDetection{"Game detection", &Dev};
	inline Section Dev_Mesh{"Meshes", &Dev};
	inline Section Dev_ImGui{"ImGui developer settings", &Dev};

	// imgui settings
	inline Tab Settings{"Settings", &Root};

	// should be last - for development to make sure that no one else creates Tabs & Sections :p
	inline Tab _DummyLast{nullptr, nullptr};

} // namespace SptImGuiGroup

class SptImGui
{
	/*
	* Only call these during PreHook() or later! These return true if the internal feature is loaded.
	* If the return value is false, the callback will *not* be called.
	* If the return value is true, the callbacks will be called at the end of each frame:
	* - tab/section callbacks will always be called, even if that tab is closed
	* - window callbacks will always be called
	* 
	* The logic for tab/section callbacks should look something like this:
	* if (open) {
	*   // draw ImGui stuff
	* }
	* // update feature internals
	* // display other ImGui windows
	* 
	* The logic for window callbacks should look something like this:
	* if (ImGui::Begin(...)) {
	*   // drawing logic
	* }
	* ImGui::End();
	* 
	* If your feature can be used from the console and you want support for many games/versions,
	* avoid relying on the ImGui callback to update the feature internals. Instead, use e.g.
	* SV_FrameSignal since that is more thoroughly tested and supported for more game versions.
	* 
	* Tab/section callbacks for the main window are called in the order that those objects are
	* defined above. Window callbacks are called in the order that the register functions were
	* called in. Since this (probably) depends on feature initialization order it should not be
	* relied on (at least until feature dependencies are implemented). The main window has no
	* special priority over other windows.
	* 
	* Each leaf section/tab should have at most a single callback. Section/tab callbacks are also
	* allowed to make their own windows (e.g. the tab has a checkbox to enable the feature, and
	* when checked the main guts of the feature are displayed in a separate window).
	*/

public:
	// true if the imgui feature works and is loaded
	static bool Loaded();
	// register a callback for an associated tab in the main window
	static bool RegisterTabCallback(SptImGuiGroup::Tab& tab, const SptImGuiTabCallback& cb);
	// register a callback for a section within a tab - use for really small or simple features that can be displayed along-side others
	static bool RegisterSectionCallback(SptImGuiGroup::Section& section, const SptImGuiSectionCallback& cb);
	// use this if you want a separate window for your feature
	static bool RegisterWindowCallback(const SptImGuiWindowCallback& cb);

	/*
	* HUD cvars should all be listed in a single tab. For simple on/off cvars, use a checkbox.
	* For anything more complicated create a draw callback - if it's more than 1 line tall, put
	* it in a collapsible. You can (and should) reuse this callback for drawing the cvar logic
	* within the dedicated tab/section for your feature.
	*/
	static void RegisterHudCvarCheckbox(ConVar& var);
	static void RegisterHudCvarCallback(ConVar& var, const SptImGuiHudTextCallback& cb, bool putInCollapsible);

	/*
	* Opens the root window & calls ImGui::SetNextWindowFocus(). There are no utility functions for
	* bringing focus to a specific tab/section because the logic for that is actually very
	* complicated. If you have a (non-niche) use case for this, let me know and I will consider
	* implementing it. See: TODO GITHUB ISSUE
	*/
	static void BringFocusToMainWindow();

	static ImGuiWindow* GetMainWindow();
	static void ToggleFeature();

	// SPT-related widgets

	enum CvarValueFlags
	{
		CVF_NONE = 0,
		CVF_ALWAYS_QUOTE = 1,
		// TODO: copy/reset widgets
	};

	// a button for a ConCommand with no args, does NOT invoke the command (because of OE compat)
	static bool CmdButton(const char* label, ConCommand& cmd);
	// display cvar name & value, surrounds the value in quotes if it has a space (if the value already has quotes, too bad!)
	static void CvarValue(const ConVar& c, CvarValueFlags flags = CVF_NONE);
	// a checkbox for a boolean cvar, returns value of cvar
	static bool CvarCheckbox(ConVar& c, const char* label);
	// a combo/dropdown box for a cvar with multiple integer options (does not use clipper - not optimized for huge lists), returns value of cvar
	static int CvarCombo(ConVar& c, const char* label, const char* const* opts, size_t nOpts);
	// a textbox for an integer in base 10 or 16, returns true if the value was modified
	static bool InputTextInteger(const char* label, const char* hint, long long& val, int radix);
	// same as internal imgui help marker - a tooltip with extra info (for cvars & commands)
	static void HelpMarker(const char* fmt, ...);
	// add a border around an item - this is just a table with a single row/column
	static bool BeginBordered(const ImVec2& outer_size = {0.0f, 0.0f}, float inner_width = 0.0f);
	static void EndBordered();

	struct AutocompletePersistData
	{
		/*
		* Values are readonly & most are for internal use. Apparently there is a plan to eventually
		* overhaul ImGui::InputText which may change how this works in the future. As a botched
		* hack, if you want to set the textInput yourself - instead of overwriting it, overwrite
		* one of the autocomplete entries and set the overwriteIdx to that entry. This will handle
		* recomputing the autocomplete entries, etc.
		*/
		char textInput[COMMAND_COMPLETION_ITEM_LENGTH]{""}; // public
		char autocomplete[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH];
		int nAutocomplete = 0;
		int autocompleteSelectIdx = 0;
		int overwriteIdx = -1;
		bool recomputeAutocomplete = true;
		bool enterPressed = false; // public
		bool modified = false;     // public
	};
	/*
	* A text input field with autocomplete for a specific ConCommand. To use, create a static
	* AutocompletePersistData and pass it in here every time you call this widget. 7.25 display
	* items is what BeginListBox() uses and is a reasonable default.
	* 
	* Textbox entries are fed to the autocomplete func as if entered from the console e.g. if the
	* textbox has "xyz", the autocomplete func will get "spt_draw_map_overlay xyz". This *should*
	* work for commands with multiple args (not tested). SPT's current implementations of
	* autocomplete functions are meant to be compatible with OE and therefore have limits of 64
	* entries & 64 characters per entry. Since I don't expect anyone to write their own
	* autocomplete functions for con commands, this widget is meant to be compatible with those
	* callbacks and therefore also has those limits.
	* 
	* If you want a more general-purpose autocomplete widget without these limits, let me know
	* and I can add another function without those restrictions.
	*/
	static void TextInputAutocomplete(const char* inputTextLabel,
	                                  const char* popupId,
	                                  AutocompletePersistData& persist,
	                                  FnCommandCompletionCallback autocompleteFn,
	                                  const char* cmdName,
	                                  float maxDisplayItems = 7.25f);
};

inline ConCommand spt_gui{
    "spt_gui",
    SptImGui::ToggleFeature,
    "Toggle the SPT GUI",
    FCVAR_DONTRECORD,
};

// Use these instead of reinterpret casts! I've enabled IMGUI_USE_BGRA_PACKED_COLOR because that
// should in theory be better for the dx9 backend, but that means color32 != ImU32.

inline color32 ImU32ToColor32(ImU32 col)
{
	return {
	    (byte)(col >> IM_COL32_R_SHIFT),
	    (byte)(col >> IM_COL32_G_SHIFT),
	    (byte)(col >> IM_COL32_B_SHIFT),
	    (byte)(col >> IM_COL32_A_SHIFT),
	};
}

inline ImU32 Color32ToImU32(color32 col)
{
	return IM_COL32(col.r, col.g, col.b, col.a);
}
