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

vector<deviceeffect>::iterator FindDevEffect(profile* prof, int devNum, int type) {
	return devNum < 0 ? prof->effects.end() : find_if(prof->effects.begin(), prof->effects.end(),
		[devNum, type](auto t) {
			return conf->afx_dev.fxdevs[devNum].pid == t.pid &&
				conf->afx_dev.fxdevs[devNum].vid == t.vid &&
				t.globalMode == type;
		});
}

void RefreshDeviceList(HWND hDlg, int devNum, profile* prof) {
	HWND dev_list = GetDlgItem(hDlg, IDC_DE_LIST);
	ListBox_ResetContent(dev_list);
	for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++)
		if (conf->afx_dev.fxdevs[i].dev && conf->afx_dev.fxdevs[i].dev->IsHaveGlobal()) {
			int ind = ListBox_AddString(dev_list, conf->afx_dev.fxdevs[i].name.c_str());
			ListBox_SetItemData(dev_list, ind, i);
			if (i == devNum) {
				ListBox_SetCurSel(dev_list, ind);
				vector<deviceeffect>::iterator b1 = FindDevEffect(prof, devNum, 1),
					b2 = FindDevEffect(prof, devNum, 2);
				switch (conf->afx_dev.fxdevs[devNum].dev->GetVersion()) {
				case 5:
					// for v5
					UpdateCombo(GetDlgItem(hDlg, IDC_GLOBAL_EFFECT),
						{ "Off", "Breathing", "Single-color Wave", "Dual-color Wave", "Pulse", "Mixed Pulse", "Night Rider", "Laser" },
						b1 == prof->effects.end() ? 0 : b1->globalEffect, { 0,2,3,4,8,9,10,11 });
					EnableWindow(GetDlgItem(hDlg, IDC_GLOBAL_KEYEFFECT), false);
					break;
				case 8:
					// for v8
					UpdateCombo(GetDlgItem(hDlg, IDC_GLOBAL_EFFECT),
						{ "Off", "Morph", "Pulse", "Back morph", "Breath", "Rainbow", "Wave", "Rainbow wave", "Circle wave", "Reset" },
						b1 == prof->effects.end() ? 0 : b1->globalEffect, { 0,1,2,3,7,8,15,16,17,19 });
					EnableWindow(GetDlgItem(hDlg, IDC_GLOBAL_KEYEFFECT), true);
					UpdateCombo(GetDlgItem(hDlg, IDC_GLOBAL_KEYEFFECT),
						{ "Off", "Morph", "Pulse", "Back morph", "Breath", "Rainbow", "Wave", "Rainbow wave", "Circle wave", "Reset" },
						b2 == prof->effects.end() ? 0 : b2->globalEffect, { 0,1,2,3,7,8,15,16,17,19 });
					break;
				}
				if (b1 != prof->effects.end()) {
					SendMessage(GetDlgItem(hDlg, IDC_SLIDER_TEMPO), TBM_SETPOS, true, b1->globalDelay);
					SetSlider(sTip1, b1->globalDelay);
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, &b1->effColor1);
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, &b1->effColor2);
				}
				else {
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, NULL);
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, NULL);
				}
				if (b2 != prof->effects.end()) {
					SendMessage(GetDlgItem(hDlg, IDC_SLIDER_KEYTEMPO), TBM_SETPOS, true, b2->globalDelay);
					SetSlider(sTip2, b2->globalDelay);
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR3, &b2->effColor1);
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR4, &b2->effColor2);
				}
				else {
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR3, NULL);
					RedrawButton(hDlg, IDC_BUTTON_EFFCLR4, NULL);
				}
			}
		}
}

BOOL CALLBACK DeviceEffectDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND dev_list = GetDlgItem(hDlg, IDC_DE_LIST),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO),
		eff_keytempo = GetDlgItem(hDlg, IDC_SLIDER_KEYTEMPO);
	profile* prof = conf->FindProfile(pCid);
	static int devNum = -1;

	vector<deviceeffect>::iterator b1 = FindDevEffect(prof, devNum, 1),
		b2 = FindDevEffect(prof, devNum, 2);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		SendMessage(eff_tempo, TBM_SETRANGE, true, MAKELPARAM(0, 0xff));
		SendMessage(eff_tempo, TBM_SETTICFREQ, 16, 0);
		sTip1 = CreateToolTip(eff_tempo, sTip1);
		SendMessage(eff_keytempo, TBM_SETRANGE, true, MAKELPARAM(0, 0xff));
		SendMessage(eff_keytempo, TBM_SETTICFREQ, 16, 0);
		sTip2 = CreateToolTip(eff_keytempo, sTip2);
		RefreshDeviceList(hDlg, devNum, prof);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCLOSE: case IDCANCEL: EndDialog(hDlg, IDCLOSE); break;
		case IDC_DE_LIST: {
			devNum = (int) ListBox_GetItemData(dev_list, ListBox_GetCurSel(dev_list));
			RefreshDeviceList(hDlg, devNum, prof);
		} break;
		case IDC_GLOBAL_EFFECT: case IDC_GLOBAL_KEYEFFECT: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				vector<deviceeffect>::iterator b = LOWORD(wParam) == IDC_GLOBAL_EFFECT ? b1 : b2;
				byte newEffect = (byte)ComboBox_GetItemData(GetDlgItem(hDlg, LOWORD(wParam)), ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam))));
				if (b != prof->effects.end())
					b->globalEffect = newEffect;
				else {
					prof->effects.push_back({ conf->afx_dev.fxdevs[devNum].vid,
						conf->afx_dev.fxdevs[devNum].pid, {0}, {0},
						newEffect, 5, (byte)(LOWORD(wParam) == IDC_GLOBAL_EFFECT ? 1 : 2) });
					b = prof->effects.end() - 1;
				}
				if (pCid == conf->activeProfile->id) {
					fxhl->UpdateGlobalEffect(conf->afx_dev.fxdevs[devNum].dev);
					fxhl->Refresh();
				}
				if (!newEffect) {
					prof->effects.erase(b);
				}
				RefreshDeviceList(hDlg, devNum, prof);
			} break;
			}
		} break;
		case IDC_BUTTON_EFFCLR1: case IDC_BUTTON_EFFCLR3:
		{
			vector<deviceeffect>::iterator b = LOWORD(wParam) == IDC_BUTTON_EFFCLR1 ? b1 : b2;
			if (b != prof->effects.end()) {
				SetColor(hDlg, LOWORD(wParam), &b->effColor1);
				if (pCid == conf->activeProfile->id)
					fxhl->UpdateGlobalEffect(conf->afx_dev.fxdevs[devNum].dev);
			}
		} break;
		case IDC_BUTTON_EFFCLR2: case IDC_BUTTON_EFFCLR4:
		{
			vector<deviceeffect>::iterator b = LOWORD(wParam) == IDC_BUTTON_EFFCLR2 ? b1 : b2;
			if (b != prof->effects.end()) {
				SetColor(hDlg, LOWORD(wParam), &b->effColor2);
				if (pCid == conf->activeProfile->id)
					fxhl->UpdateGlobalEffect(conf->afx_dev.fxdevs[devNum].dev);
			}
		} break;
		} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		case IDC_BUTTON_EFFCLR1: case IDC_BUTTON_EFFCLR2:
			if (b1 != prof->effects.end()) {
				RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID,
					((DRAWITEMSTRUCT*)lParam)->CtlID  == IDC_BUTTON_EFFCLR1 ? &b1->effColor1 : &b1->effColor2);
			}
			break;
		case IDC_BUTTON_EFFCLR3: case IDC_BUTTON_EFFCLR4:
			if (b2 != prof->effects.end()) {
				RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID,
					((DRAWITEMSTRUCT*)lParam)->CtlID == IDC_BUTTON_EFFCLR3 ? &b2->effColor1 : &b2->effColor2);
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
				vector<deviceeffect>::iterator b = (HWND)lParam == eff_tempo ? b1 : b2;
				if (b != prof->effects.end()) {
					b->globalDelay = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider((HWND)lParam == eff_tempo ? sTip1 : sTip2, b->globalDelay);
					if (prof->id == conf->activeProfile->id)
						fxhl->UpdateGlobalEffect(conf->afx_dev.fxdevs[devNum].dev);
				}
			} break;
			}
	} break;
	default: return false;
	}
	return true;
}

void ReloadKeyList(HWND hDlg, WORD key) {
	HWND key_list = GetDlgItem(hDlg, IDC_COMBO_TRIGGERKEY);
	UpdateCombo(key_list,
		{ "Off", "Left Shift", "Left Control", "Left Alt", "Windows key", "Right Shift", "Right Control", "Right Alt" }, key,
		{ 0, VK_LSHIFT, VK_LCONTROL, VK_LMENU, VK_LWIN, VK_RSHIFT, VK_RCONTROL, VK_RMENU });
}

void ReloadProfSettings(HWND hDlg, profile *prof) {
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		mode_list = GetDlgItem(hDlg, IDC_COMBO_EFFMODE);
	CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof->flags & PROF_DEFAULT);
	CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, prof->flags & PROF_PRIORITY);
	CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof->flags & PROF_DIMMED);
	CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, prof->flags & PROF_ACTIVE);
	CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, prof->flags & PROF_FANS);

	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_AC, prof->triggerFlags & PROF_TRIGGER_AC);
	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_BATTERY, prof->triggerFlags & PROF_TRIGGER_BATTERY);

	ReloadModeList(mode_list, prof->effmode);
	ReloadKeyList(hDlg, prof->triggerkey);
	ListBox_ResetContent(app_list);
	for (int j = 0; j < prof->triggerapp.size(); j++)
		ListBox_AddString(app_list, prof->triggerapp[j].c_str());
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
		key_list = GetDlgItem(hDlg, IDC_COMBO_TRIGGERKEY),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO);

	profile *prof = conf->FindProfile(pCid);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		pCid = conf->activeProfile ? conf->activeProfile->id : conf->FindDefaultProfile()->id;
		ReloadProfileView(hDlg);
	} break;
	case WM_COMMAND:
	{
		if (!prof && LOWORD(wParam) != IDC_ADDPROFILE)
			return false;

		WORD state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;

		switch (LOWORD(wParam))
		{
		case IDC_DEV_EFFECT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_DEVICEEFFECTS), hDlg, (DLGPROC)DeviceEffectDialog);
			break;
		case IDC_COMBO_TRIGGERKEY: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				prof->triggerkey = (WORD)ComboBox_GetItemData(key_list, ComboBox_GetCurSel(key_list));
			} break;
			}
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
				if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					conf->profiles.erase(find_if(conf->profiles.begin(), conf->profiles.end(),
						[](profile* pr) {
							return pr->id == pCid;
						}));
					// is this active profile? Switch needed!
					if (conf->activeProfile->id == pCid) {
						// switch to default profile..
						eve->SwitchActiveProfile(conf->FindDefaultProfile());
						pCid = conf->activeProfile->id;
					}
					delete prof;
					RemoveUnusedGroups();
					ReloadProfileView(hDlg);
					ReloadProfileList();
				}
			}
			else
				ShowNotification(&conf->niData, "Error", "Can't delete last or default profile!", true);
		} break;
		case IDC_BUT_PROFRESET:
			if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you really want to remove selected light settings from this profile?", "Warning",
										   MB_YESNO | MB_ICONWARNING) == IDYES) {
				for (auto it = prof->lightsets.begin(); it < prof->lightsets.end(); it++) {
					if (IsDlgButtonChecked(hDlg, IDC_CP_COLORS) == BST_CHECKED)
						it->color.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_EVENTS) == BST_CHECKED)
						it->events.clear();
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
					prof->fansets = { };
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
						lset->events = t->events;
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
				auto old_def = find_if(conf->profiles.begin(), conf->profiles.end(),
					[](profile* prof) {
						return (prof->flags & PROF_DEFAULT);
					});
				(*old_def)->flags = (*old_def)->flags & ~PROF_DEFAULT;
				prof->flags |= PROF_DEFAULT;
			} else
				CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_CHECKED);
		} break;
		case IDC_CHECK_PRIORITY:
			SetBitMask(prof->flags, PROF_PRIORITY, state);
			// Update active profile, if needed
			if (conf->enableProf)
				eve->ScanTaskList();
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
				if (lPoint->uNewState & LVIS_SELECTED && lPoint->iItem != -1) {
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
					ListView_SetItem(((NMHDR*)lParam)->hwndFrom, &sItem->item);
					ReloadProfileList();
				}
			} break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}