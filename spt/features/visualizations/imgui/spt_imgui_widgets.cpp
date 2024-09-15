#include <stdafx.hpp>
#include "imgui_interface.hpp"
#include "thirdparty/imgui/imgui_internal.h"

bool SptImGui::CmdButton(const char* label, ConCommand& cmd)
{
	bool ret = ImGui::Button(label);
	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", WrangleLegacyCommandName(cmd.GetName(), true, nullptr));
	ImGui::SameLine();
	HelpMarker("%s", cmd.GetHelpText());
	return ret;
}

void SptImGui::CvarValue(const ConVar& c)
{
	const char* v = c.GetString();
	const char* surround = "";
	if (strchr(v, ' '))
		surround = " ";
	ImGui::TextDisabled("(%s %s%s%s)", WrangleLegacyCommandName(c.GetName(), true, nullptr), surround, v, surround);
	ImGui::SameLine();
	HelpMarker("%s", c.GetHelpText());
}

bool SptImGui::CvarCheckbox(ConVar& c, const char* label)
{
	bool oldVal = c.GetBool();
	bool newVal = oldVal;
	ImGui::Checkbox(label, &newVal);
	if (oldVal != newVal)
		c.SetValue(newVal); // only set value if it was changed to not spam cvar callbacks
	ImGui::SameLine();
	CvarValue(c);
	return newVal;
}

int SptImGui::CvarCombo(ConVar& c, const char* label, const char* const* opts, size_t nOpts)
{
	Assert(nOpts >= 1);
	int val = clamp(c.GetInt(), 0, (int)nOpts - 1);

	if (ImGui::BeginCombo(label, opts[val], ImGuiComboFlags_WidthFitPreview))
	{
		for (int i = 0; i < (int)nOpts; i++)
		{
			const bool is_selected = (val == i);
			if (ImGui::Selectable(opts[i], is_selected))
			{
				val = i;
				// set value even if it wasn't "changed" to apply clamp (e.g. was 4 but nOpts is 3 and value was set to 2)
				c.SetValue(i);
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	SptImGui::CvarValue(c);
	return val;
}

void SptImGui::HelpMarker(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(-1.f);
		ImGui::TextV(fmt, va);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
	va_end(va);
}

void SptImGui::TextInputAutocomplete(const char* inputTextLabel,
                                     const char* popupId,
                                     AutocompletePersistData& persist,
                                     FnCommandCompletionCallback autocompleteFn,
                                     const char* cmdName,
                                     float maxDisplayItems)
{
	/*
	* Unfortunately ImGui does not (currently) have a InputText with autocomplete, so I tried to
	* make one myself. The feature is discussed here: https://github.com/ocornut/imgui/issues/718.
	* Whenever the text box is focused, a new window is created which lists the autocomplete options.
	* Instead of using BeginListBox() directly, I set the window size and use clipper instead. In
	* theory, that means that this can be expanded in the future to support much more than the default
	* 64 items (that source uses) and it should still be efficient (tm).
	* 
	* There are a few nuances to the input handling that I haven't figured out how to account for
	* gracefully. The tricky bit is that I want to allow both keyboard TAB autocomplete and selecting
	* the autocomplete entry with the mouse. Keyboard events by themselves are simple enough:
	* 
	* - each edit to the text field recomputes the autocomplete entries
	* - pressing TAB fills the text field with the selected entry (and recomputes autocomplete)
	* - UP/DOWN arrows change the selected item and force the scroll bar to move
	* 
	* Using the mouse on its own is also pretty simple. The only mildly annoying thing I haven't
	* figured out is when you interact with the popup window with the mouse, that loses focus on the
	* text box and you need to click on it again to start typing. If we select an autocomplete entry
	* with the mouse, then we can use SetActiveID() to refocus the text box, but if we start using
	* the scroll bar all hope is lost. Using SetActiveID() on the text box in that case will prevent
	* the scroll bar from getting used.
	*/

	struct AutoCompleteUserData
	{
		AutocompletePersistData& persist;
		FnCommandCompletionCallback autocompleteFn;
		const char* cmdName;
		bool forceScroll;
	} udata{
	    .persist = persist,
	    .autocompleteFn = autocompleteFn,
	    .cmdName = cmdName,
	    .forceScroll = false,
	};

	if (persist.recomputeAutocomplete)
	{
		persist.nAutocomplete = udata.autocompleteFn(persist.textInput, persist.autocomplete);
		persist.recomputeAutocomplete = false;
	}

	ImGuiInputTextFlags textFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackHistory
	                                | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackEdit
	                                | ImGuiInputTextFlags_EnterReturnsTrue;
	// The only way I've been able to overwrite the buffer (e.g. mouse pressed on autocomplete entry)
	// is within the callback; setting the textInput directly isn't currently supported by ImGui ;}.
	if (persist.overwriteIdx >= 0)
		textFlags |= ImGuiInputTextFlags_CallbackAlways;
	persist.modified = false;

	persist.enterPressed = ImGui::InputText(
	    inputTextLabel,
	    persist.textInput,
	    sizeof persist.textInput,
	    textFlags,
	    [](ImGuiInputTextCallbackData* data)
	    {
		    AutoCompleteUserData& ud = *(AutoCompleteUserData*)data->UserData;
		    AutocompletePersistData& pd = ud.persist;
		    switch (data->EventFlag)
		    {
		    case ImGuiInputTextFlags_CallbackAlways:
			    if (pd.overwriteIdx < 0)
				    break;
			    [[fallthrough]];
		    case ImGuiInputTextFlags_CallbackCompletion:
			    strncpy(data->Buf,
			            pd.autocomplete[pd.overwriteIdx >= 0 ? pd.overwriteIdx : pd.autocompleteSelectIdx],
			            data->BufSize);
			    pd.overwriteIdx = -1;
			    data->BufTextLen = data->CursorPos = strnlen(data->Buf, data->BufSize);
			    data->SelectionStart = data->SelectionEnd;
			    data->BufDirty = true;
			    [[fallthrough]];
		    case ImGuiInputTextFlags_CallbackEdit:
		    {
			    /*
				* Feed data to the completion callback as if it was called from the console - the
				* cmd name is added before the textbox input and dropped from the autocomplete
				* results. If we change this to a more generic (non ConCommand) based function,
				* feed the textbox input directly to the autocomplete func instead of prefixing it.
				*/
			    static char tmpBuf[SPT_MAX_CVAR_NAME_LEN + COMMAND_COMPLETION_ITEM_LENGTH + 2];
			    snprintf(tmpBuf, sizeof tmpBuf, "%s %s", ud.cmdName, data->Buf);
			    tmpBuf[sizeof(tmpBuf) - 1] = '\0';
			    pd.nAutocomplete = ud.autocompleteFn(tmpBuf, pd.autocomplete);
			    size_t nTrim = strlen(ud.cmdName) + 1;
			    for (int i = 0; i < pd.nAutocomplete; i++)
				    memmove(pd.autocomplete[i], pd.autocomplete[i] + nTrim, data->BufSize - nTrim);
			    pd.autocompleteSelectIdx = 0;
			    pd.modified = true;
			    break;
		    }
		    case ImGuiInputTextFlags_CallbackHistory:
			    // The history flag was meant for getting previously entered values,
			    // we're using it to notify us when up/down is pressed.
			    pd.autocompleteSelectIdx += data->EventKey == ImGuiKey_UpArrow
			                                    ? -1
			                                    : (data->EventKey == ImGuiKey_DownArrow ? 1 : 0);
			    ud.forceScroll = true;
			    break;
		    }
		    if (pd.nAutocomplete != 0)
			    pd.autocompleteSelectIdx = (pd.autocompleteSelectIdx + pd.nAutocomplete) % pd.nAutocomplete;
		    return 0;
	    },
	    &udata);

	ImGuiWindow* textBoxWnd = ImGui::GetCurrentWindow();
	ImGuiID textBoxId = ImGui::GetID(inputTextLabel);
	ImGuiWindow* navWindow = ImGui::GetCurrentContext()->NavWindow;

	bool popupIsNav = navWindow && !strcmp(popupId, navWindow->Name);

	// determine if we should display the "popup"

	// no focused window or focused window is not root/popup window
	if (!navWindow || (navWindow != textBoxWnd && !popupIsNav))
		return;
	// nothing to autocomplete
	if (persist.nAutocomplete == 0)
		return; 
	/*
	* - if the textbox is the active element display the popup
	* - if the popup window is the nav window - great, keep it alive
	* - the third condition is to handle the one frame transition between those two states:
	*   (the textbox is not active AND the popup is not the nav window)
	*/
	if (!(popupIsNav || ImGui::IsItemActive() || ImGui::IsItemDeactivated()))
		return;
	// 1 entry to autocomplete but it's the same as what user typed in the text box
	if (persist.nAutocomplete == 1 && !strcmp(persist.autocomplete[0], persist.textInput))
		return; 

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
	float nDisplayItems, framePadding;
	if (persist.nAutocomplete <= maxDisplayItems)
	{
		nDisplayItems = persist.nAutocomplete;
		framePadding = 0;
	}
	else
	{
		nDisplayItems = maxDisplayItems;
		framePadding = ImGui::GetStyle().FramePadding.y * 2.0f;
	}
	ImGui::SetNextWindowSize({
	    ImGui::GetItemRectSize().x,
	    ImGui::GetTextLineHeightWithSpacing() * nDisplayItems + framePadding,
	});
	auto& style = ImGui::GetStyle();
	// align clipper entries with the text in the textbox
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {style.FramePadding.x, style.ItemSpacing.y * .5f});
	ImGuiWindowFlags wndFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
	                            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
	                            | ImGuiWindowFlags_NoFocusOnAppearing;

	if (ImGui::Begin(popupId, nullptr, wndFlags))
	{
		ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

		ImGuiListClipper clipper;
		clipper.Begin(persist.nAutocomplete);
		clipper.IncludeItemByIndex(persist.autocompleteSelectIdx); // to allow ScrollToItem()
		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				ImGui::PushID(i);
				if (ImGui::Selectable(persist.autocomplete[i], persist.autocompleteSelectIdx == i))
				{
					persist.overwriteIdx = i;
					ImGui::SetActiveID(textBoxId, textBoxWnd);
				}
				if (udata.forceScroll && persist.autocompleteSelectIdx == i)
					ImGui::ScrollToItem();
				ImGui::PopID();
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
}
