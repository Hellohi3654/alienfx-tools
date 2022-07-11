#include "alienfx-gui.h"
#include "EventHandler.h"
#include "common.h"
#include <Shlwapi.h>

extern void ReloadProfileList();
extern void ReloadModeList(HWND dlg=NULL, int mode = conf->GetEffect());
extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern void RemoveUnused(vector<groupset>* lightsets);
extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);

extern EventHandler* eve;
int pCid = -1;

void ReloadKeyList(HWND hDlg, WORD key) {
	HWND key_list = GetDlgItem(hDlg, IDC_COMBO_TRIGGERKEY);
	UpdateCombo(key_list,
		{ "Off", "Left Shift", "Left Control", "Left Alt", "Windows key", "Right Shift", "Right Control", "Right Alt" }, key,
		{ 0, VK_LSHIFT, VK_LCONTROL, VK_LMENU, VK_LWIN, VK_RSHIFT, VK_RCONTROL, VK_RMENU });
}

void ReloadProfSettings(HWND hDlg, profile *prof) {
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		eff_list = GetDlgItem(hDlg, IDC_GLOBAL_EFFECT),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO),
		mode_list = GetDlgItem(hDlg, IDC_COMBO_EFFMODE);
	CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof->flags & PROF_DEFAULT);
	CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, prof->flags & PROF_PRIORITY);
	CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof->flags & PROF_DIMMED);
	CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, prof->flags & PROF_ACTIVE);
	CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, prof->flags & PROF_FANS);

	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_AC, prof->triggerFlags & PROF_TRIGGER_AC);
	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_BATTERY, prof->triggerFlags & PROF_TRIGGER_BATTERY);

	//CheckDlgButton(hDlg, IDC_CHECK_GLOBAL, prof->flags & PROF_GLOBAL_EFFECTS);
	ReloadModeList(mode_list, prof->effmode);
	ReloadKeyList(hDlg, prof->triggerkey);
	//ComboBox_SetCurSel(mode_list, prof->effmode);
	ListBox_ResetContent(app_list);
	for (int j = 0; j < prof->triggerapp.size(); j++)
		ListBox_AddString(app_list, prof->triggerapp[j].c_str());
	// set global effect, colors and delay
	bool flag = prof->effmode == 99;// &PROF_GLOBAL_EFFECTS;
	EnableWindow(eff_list, flag);
	EnableWindow(eff_tempo, flag);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), flag);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), flag);
	// now sliders...
	SendMessage(eff_tempo, TBM_SETPOS, true, prof->globalDelay);
	SetSlider(sTip2, prof->globalDelay);
	// now colors...
	RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, flag ? &prof->effColor1 : 0);
	RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, flag ? &prof->effColor2 : 0);
	ComboBox_SetCurSel(eff_list, prof->globalEffect);
}

void ReloadProfileView(HWND hDlg) {
	int rpos = 0;
	HWND profile_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);
	ListView_DeleteAllItems(profile_list);
	ListView_SetExtendedListViewStyle(profile_list, LVS_EX_FULLROWSELECT);
	LVCOLUMNA lCol{ LVCF_WIDTH, LVCFMT_LEFT, 100 };
	ListView_DeleteColumn(profile_list, 0);
	ListView_InsertColumn(profile_list, 0, &lCol);
	for (int i = 0; i < conf->profiles.size(); i++) {
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i};
		lItem.lParam = conf->profiles[i]->id;
		lItem.pszText = (char*)conf->profiles[i]->name.c_str();
		if (conf->profiles[i]->id == pCid) {
			lItem.state = LVIS_SELECTED;
			ReloadProfSettings(hDlg, conf->profiles[i]);
			rpos = i;
		}
		else
			lItem.state = 0;
		ListView_InsertItem(profile_list, &lItem);
	}
	ListView_SetColumnWidth(profile_list, 0, LVSCW_AUTOSIZE);
	ListView_EnsureVisible(profile_list, rpos, false);
}

void RemoveUnusedGroups() {
	for (int i = 0; i < conf->afx_dev.GetGroups()->size(); i++) {
		DWORD gid = conf->afx_dev.GetGroups()->at(i).gid;
		if (find_if(conf->profiles.begin(), conf->profiles.end(),
			[gid](profile* cp) {
				return find_if(cp->lightsets.begin(), cp->lightsets.end(),
					[gid](groupset t) {
						return t.group == gid;
					}) != cp->lightsets.end();
			}) == conf->profiles.end()) {
			conf->afx_dev.GetGroups()->erase(conf->afx_dev.GetGroups()->begin() + i);
			i--;
		}
	}

}

BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		mode_list = GetDlgItem(hDlg, IDC_COMBO_EFFMODE),
		eff_list = GetDlgItem(hDlg, IDC_GLOBAL_EFFECT),
		key_list = GetDlgItem(hDlg, IDC_COMBO_TRIGGERKEY),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO);

	profile *prof = conf->FindProfile(pCid);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		pCid = conf->activeProfile ? conf->activeProfile->id : conf->FindDefaultProfile()->id;
		//CheckDlgButton(hDlg, IDC_CP_COLORS, true);
		//CheckDlgButton(hDlg, IDC_CP_EVENTS, true);
		//CheckDlgButton(hDlg, IDC_CP_AMBIENT, true);
		//CheckDlgButton(hDlg, IDC_CP_HAPTICS, true);
		//CheckDlgButton(hDlg, IDC_CP_FANS, true);
		if (conf->haveV5) {
			UpdateCombo(eff_list,
				{ "Color", "Breathing", "Single-color Wave", "Dual-color Wave", "Pulse", "Mixed Pulse", "Night Rider", "Laser" },
				0, {0,2,3,4,8,9,10,11});
			SendMessage(eff_tempo, TBM_SETRANGE, true, MAKELPARAM(0, 0xa));
			SendMessage(eff_tempo, TBM_SETTICFREQ, 1, 0);
			sTip2 = CreateToolTip(eff_tempo, sTip2);
		}

		ReloadProfileView(hDlg);
	} break;
	case WM_COMMAND:
	{
		if (!prof && LOWORD(wParam) != IDC_ADDPROFILE)
			return false;

		WORD state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;

		switch (LOWORD(wParam))
		{
		case IDC_GLOBAL_EFFECT: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				prof->globalEffect = (byte) ComboBox_GetItemData(eff_list, ComboBox_GetCurSel(eff_list));
				if (prof->id == conf->activeProfile->id)
					fxhl->UpdateGlobalEffect();
			} break;
			}
		} break;
		case IDC_COMBO_TRIGGERKEY: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				prof->triggerkey = (WORD)ComboBox_GetItemData(key_list, ComboBox_GetCurSel(key_list));
			} break;
			}
		} break;
		case IDC_BUTTON_EFFCLR1:
		{
			SetColor(hDlg, IDC_BUTTON_EFFCLR1, &prof->effColor1);
			if (prof->id == conf->activeProfile->id)
				fxhl->UpdateGlobalEffect();
		} break;
		case IDC_BUTTON_EFFCLR2:
		{
			SetColor(hDlg, IDC_BUTTON_EFFCLR2, &prof->effColor2);
			if (prof->id == conf->activeProfile->id)
				fxhl->UpdateGlobalEffect();
		} break;
		case IDC_ADDPROFILE: {
			unsigned vacID = 0;
			for (int i = 0; i < conf->profiles.size(); i++)
				if (vacID == conf->profiles[i]->id) {
					vacID++; i = -1;
				}
			if (!prof)
				prof = conf->activeProfile;
			profile* new_prof = new profile(*prof);
			new_prof->id = vacID;
			new_prof->flags &= ~PROF_DEFAULT;
			new_prof->name = "Profile " + to_string(vacID);
			conf->profiles.push_back(new_prof);
			pCid = vacID;
			ReloadProfileView(hDlg);
			ReloadProfileList();
		} break;
		case IDC_REMOVEPROFILE: {
			if (!(prof->flags & PROF_DEFAULT) && conf->profiles.size() > 1) {
				int pdelID = pCid;
				if (MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					// is this active profile? Switch needed!
					if (conf->activeProfile->id == pCid) {
						// switch to default profile..
						eve->SwitchActiveProfile(conf->FindDefaultProfile());
						pCid = conf->FindDefaultProfile()->id;
						ReloadProfileList();
					}
					conf->profiles.erase(find_if(conf->profiles.begin(), conf->profiles.end(),
						[pdelID](profile* pr) {
							return pr->id == pdelID;
						}));
					RemoveUnusedGroups();
					ReloadProfileView(hDlg);
				}
			}
			else
				ShowNotification(&conf->niData, "Error", "Can't delete last or default profile!", true);
		} break;
		case IDC_BUT_PROFRESET:
			if (MessageBox(hDlg, "Do you really want to remove selected light settings from this profile?", "Warning",
										   MB_YESNO | MB_ICONWARNING) == IDYES) {
				for (auto it = prof->lightsets.begin(); it < prof->lightsets.end(); it++) {
					if (IsDlgButtonChecked(hDlg, IDC_CP_COLORS) == BST_CHECKED)
						it->color.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_EVENTS) == BST_CHECKED)
						it->events[0].state = it->events[1].state = it->events[2].state = false;
					if (IsDlgButtonChecked(hDlg, IDC_CP_AMBIENT) == BST_CHECKED)
						it->ambients.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_HAPTICS) == BST_CHECKED)
						it->haptics.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_GRID) == BST_CHECKED)
						it->effect.type = 0;
				}
				RemoveUnused(&prof->lightsets);
				RemoveUnusedGroups();
				if (IsDlgButtonChecked(hDlg, IDC_CP_FANS) == BST_CHECKED) {
					prof->fansets = { 0 };
				}
				if (conf->activeProfile->id == prof->id)
					fxhl->Refresh();
			}
			break;
		case IDC_BUT_COPYACTIVE:
			if (conf->activeProfile->id != prof->id) {
				for (auto t = conf->activeProfile->lightsets.begin(); t < conf->activeProfile->lightsets.end(); t++) {
					groupset* lset = FindMapping(t->group, &prof->lightsets);
					if (!lset) {
						prof->lightsets.push_back({ t->group });
						lset = &prof->lightsets.back();
						lset->fromColor = t->fromColor;
						lset->gauge = t->gauge;
						lset->flags = t->flags;
					}
					if (IsDlgButtonChecked(hDlg, IDC_CP_COLORS) == BST_CHECKED)
						lset->color = t->color;
					if (IsDlgButtonChecked(hDlg, IDC_CP_EVENTS) == BST_CHECKED)
						for (int i = 0; i < 3; i++)
							lset->events[i] = t->events[i];
					if (IsDlgButtonChecked(hDlg, IDC_CP_AMBIENT) == BST_CHECKED)
						lset->ambients = t->ambients;
					if (IsDlgButtonChecked(hDlg, IDC_CP_HAPTICS) == BST_CHECKED)
						lset->haptics = t->haptics;
					if (IsDlgButtonChecked(hDlg, IDC_CP_GRID) == BST_CHECKED)
						lset->effect = t->effect;
				}
				if (IsDlgButtonChecked(hDlg, IDC_CP_FANS) == BST_CHECKED)
					if (conf->activeProfile->flags & PROF_FANS)
						prof->fansets = conf->activeProfile->fansets;
					else
						prof->fansets = conf->fan_conf->prof;
			}
			break;
		case IDC_APP_RESET:
		{
			int ind = ListBox_GetCurSel(app_list);
			if (ind >= 0) {
				ListBox_DeleteString(app_list, ind);
				prof->triggerapp.erase(prof->triggerapp.begin() + ind);
			}
		} break;
		case IDC_APP_BROWSE: {
			// fileopen dialogue...
			OPENFILENAMEA fstruct{ sizeof(OPENFILENAMEA), hDlg, hInst, "Applications (*.exe)\0*.exe\0\0" };
			char appName[4096]; appName[0] = 0;
			fstruct.lpstrFile = (LPSTR) appName;
			fstruct.nMaxFile = 4095;
			fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
			if (GetOpenFileNameA(&fstruct)) {
				PathStripPath(fstruct.lpstrFile);
				prof->triggerapp.push_back(appName);
				ListBox_AddString(app_list, prof->triggerapp.back().c_str());
			}
		} break;
		case IDC_COMBO_EFFMODE:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				prof->effmode = (WORD)ComboBox_GetItemData(mode_list, ComboBox_GetCurSel(mode_list));
				if (prof->id == conf->activeProfile->id) {
					eve->ChangeEffectMode();
					ReloadModeList();
				}
			} break;
			}
		} break;
		case IDC_CHECK_DEFPROFILE:
		{
			if (state) {
				profile *old_def = conf->FindDefaultProfile();
				old_def->flags = old_def->flags & ~PROF_DEFAULT;
				prof->flags |= PROF_DEFAULT;
				if (conf->enableProf && old_def->id == conf->activeProfile->id)
					// need to switch to this
					eve->SwitchActiveProfile(prof);
			} else
				CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_CHECKED);
		} break;
		case IDC_CHECK_PRIORITY:
			SetBitMask(prof->flags, PROF_PRIORITY, state);
			// Update active profile, if needed
			if (conf->enableProf)
				eve->SwitchActiveProfile(eve->ScanTaskList());
			break;
		case IDC_CHECK_PROFDIM:
			SetBitMask(prof->flags, PROF_DIMMED, state);
			prof->ignoreDimming = false;
			if (prof->id == conf->activeProfile->id) {
				fxhl->ChangeState();
			}
			break;
		case IDC_CHECK_FOREGROUND:
			SetBitMask(prof->flags, PROF_ACTIVE, state);
			break;
		case IDC_CHECK_FANPROFILE:
			SetBitMask(prof->flags, PROF_FANS, state);
			if (prof->flags & PROF_FANS) {
				// add current fan profile...
				if (prof->fansets.fanControls.empty())
					prof->fansets = conf->fan_conf->prof;
			} else {
				// Switch fan control if needed
				if (prof->id == conf->activeProfile->id)
					conf->fan_conf->lastProf = &conf->fan_conf->prof;
			}
			break;
		case IDC_TRIGGER_POWER_AC:
			SetBitMask(prof->triggerFlags, PROF_TRIGGER_AC, state);
			break;
		case IDC_TRIGGER_POWER_BATTERY:
			SetBitMask(prof->triggerFlags, PROF_TRIGGER_BATTERY, state);
			break;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_PROFILES:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE: {
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, ((NMITEMACTIVATE*)lParam)->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & LVIS_FOCUSED && lPoint->iItem != -1) {
					// Select other item...
					pCid = (int) lPoint->lParam;
					ReloadProfSettings(hDlg, conf->FindProfile(pCid));
				} else {
					pCid = -1;
					CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, BST_UNCHECKED);
					ListBox_ResetContent(app_list);
					ComboBox_SetCurSel(mode_list, 0);
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				profile* prof = conf->FindProfile((int)sItem->item.lParam);
				if (prof && sItem->item.pszText) {
					prof->name = sItem->item.pszText;
					//ListView_SetItem(((NMHDR*)lParam)->hwndFrom, &sItem->item);
					ReloadProfileList();
					return true;
				} else
					return false;
			} break;
			}
			break;
		}
		break;
	case WM_HSCROLL:
	{
		if (prof)
			switch (LOWORD(wParam)) {
			case TB_THUMBPOSITION: case TB_ENDTRACK:
			{
				if ((HWND) lParam == eff_tempo) {
					prof->globalDelay = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, prof->globalDelay);
					if (prof->id == conf->activeProfile->id)
						fxhl->UpdateGlobalEffect();
				}
			} break;
			}
	} break;
	case WM_DRAWITEM:
	{
		if (prof /*&& (prof->flags & PROF_GLOBAL_EFFECTS)*/)
			switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
			case IDC_BUTTON_EFFCLR2:
			{
				RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, &prof->effColor1);
				RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, &prof->effColor2);
				break;
			}
			}
	} break;
	default: return false;
	}
	return true;
}