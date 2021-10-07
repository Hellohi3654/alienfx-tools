#include "alienfx-gui.h"

VOID OnSelChanged(HWND hwndDlg);
void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b);
HWND CreateToolTip(HWND hwndParent, HWND oldTip);
void SetSlider(HWND tt, int value);
int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);

extern int eItem;

void DrawFreq(HWND hDlg, int *freq) {
	unsigned i, rectop;
	char szSize[100]; //for labels

	HWND hysto = GetDlgItem(hDlg, IDC_LEVELS);

	if (hysto) {
		RECT levels_rect, graphZone;
		GetClientRect(hysto, &levels_rect);
		graphZone = levels_rect;

		HDC hdc_r = GetDC(hysto);

		// Double buff...
		HDC hdc = CreateCompatibleDC(hdc_r);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, graphZone.right - graphZone.left, graphZone.bottom - graphZone.top);

		SetBkMode(hdc, TRANSPARENT);

		HGDIOBJ hOld = SelectObject(hdc, hbmMem);

		//setting colors:
		//SetDCBrushColor(hdc, RGB(255, 255, 255));
		//SetDCPenColor(hdc, RGB(255, 255, 39));
		SetTextColor(hdc, RGB(255, 255, 255));
		//SelectObject(hdc, GetStockObject(DC_BRUSH));
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		SetBkMode(hdc, TRANSPARENT);

		if (conf->hap_conf->showAxis) {
			//draw x axis:
			MoveToEx(hdc, 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, levels_rect.right - 10, levels_rect.bottom - 21);
			LineTo(hdc, levels_rect.right - 15, levels_rect.bottom - 26);
			MoveToEx(hdc, levels_rect.right - 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, levels_rect.right - 15, levels_rect.bottom - 16);
			//TextOut(hdc, rcClientP->right - 45, rcClientP->bottom - 27, "f(kHz)", 6);

			//draw y axis:
			MoveToEx(hdc, 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, 10, 10);
			LineTo(hdc, 15, 15);
			MoveToEx(hdc, 10, 10, (LPPOINT) NULL);
			LineTo(hdc, 5, 15);
			//TextOut(hdc, 15, 10, "[Power]", 7);
			//wsprintf(szSize, "%6d", (int)y_scale);
			//TextOut(hdc, 150, 10, szSize, 6);
			//TextOut(hdc, 10, 40, "255", 3);
			//TextOut(hdc, 10, (rcClientP->bottom) / 2, "128", 3);
			//TextOut(hdc, 10, rcClientP->bottom - 35, "  0", 3);
			//axis_draw = false;
			int oldvalue = (-1);
			double coeff = 22 / (log(22.0));
			for (i = 0; i <= 22; i++) {
				int frq = int(22 - round((log(22.0 - i) * coeff)));
				if (frq > oldvalue) {
					wsprintf(szSize, "%2d", frq);
					TextOut(hdc, ((levels_rect.right - 20) * i) / 22 + 10, levels_rect.bottom - 20, szSize, 2);
					oldvalue = frq;
				}
			}
			graphZone.top = 10;
			graphZone.bottom -= 21;
			graphZone.left = 10;
			graphZone.right -= 20;
		} else {
			graphZone.top = 2;
			graphZone.left = 1;
			graphZone.bottom--;
		}
		if (freq)
			for (i = 0; i < conf->hap_conf->numbars; i++) {
				rectop = (255 - freq[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top;
				Rectangle(hdc, (graphZone.right * i) / conf->hap_conf->numbars + graphZone.left, rectop,
						  (graphZone.right * (i + 1)) / conf->hap_conf->numbars - 2 + graphZone.left, graphZone.bottom);
				//wsprintf(szSize, "%3d", freq[i]);
				//TextOut(hdc, ((rcClientP->right - 20) * i) / config->numbars + 10, rectop - 15, szSize, 3);
			}

		BitBlt(hdc_r, 0, 0, levels_rect.right - levels_rect.left, levels_rect.bottom - levels_rect.top, hdc, 0, 0, SRCCOPY);

		// Free-up the off-screen DC
		SelectObject(hdc, hOld);

		DeleteObject(hbmMem);
		DeleteDC(hdc);
		ReleaseDC(hysto, hdc_r);
		DeleteDC(hdc_r);
	}
}

bool SetColor(HWND hDlg, int id, BYTE *r, BYTE *g, BYTE *b) {
	CHOOSECOLOR cc;
	static COLORREF acrCustClr[16];
	bool ret;
	// Initialize CHOOSECOLOR 
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hDlg;
	cc.lpCustColors = (LPDWORD) acrCustClr;
	cc.rgbResult = RGB(*r, *g, *b);
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ret = ChooseColor(&cc)) {
		*r = cc.rgbResult & 0xff;
		*g = cc.rgbResult >> 8 & 0xff;
		*b = cc.rgbResult >> 16 & 0xff;
	}
	RedrawButton(hDlg, id, *r, *g, *b);
	return ret;
}

haptics_map *FindMapping(int lid) {
	if (lid != -1) {
		if (lid > 0xffff) {
			// group
			for (int i = 0; i < conf->hap_conf->mappings.size(); i++)
				if (conf->hap_conf->mappings[i].devid == 0 && conf->hap_conf->mappings[i].lightid == lid) {
					return &conf->hap_conf->mappings[i];
				}
		} else {
			// mapping
			AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(lid);
			for (int i = 0; i < conf->hap_conf->mappings.size(); i++)
				if (conf->hap_conf->mappings[i].devid == lgh.devid && conf->hap_conf->mappings[i].lightid == lgh.lightid)
					return &conf->hap_conf->mappings[i];
		}
	}
	return NULL;
}

BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND freq_list = GetDlgItem(hDlg, IDC_FREQ);
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
	HWND hLowSlider = GetDlgItem(hDlg, IDC_SLIDER_LOWCUT);
	HWND hHiSlider = GetDlgItem(hDlg, IDC_SLIDER_HICUT);
	haptics_map *map = NULL;

	switch (message) {
	case WM_INITDIALOG:
	{
		if (UpdateLightList(light_list, fxhl, 3) < 0) {
			// no lights, switch to setup
			HWND tab_list = GetParent(hDlg);
			TabCtrl_SetCurSel(tab_list, 6);
			OnSelChanged(tab_list);
			return false;
		}

		double coeff = 22030 / log(21);
		int prevfreq = 20;
		for (int i = 1; i < 21; i++) {
			int frq = 22050 - (int) round((log(21 - i) * coeff));
			string frqname = to_string(prevfreq) + "-" + to_string(frq) + "Hz";
			//sprintf_s(frqname, 55, "%d-%dHz", prevfreq, frq);
			prevfreq = frq;
			SendMessage(freq_list, LB_ADDSTRING, 0, (LPARAM) frqname.c_str());
		}

		//UpdateLightList<FXHelper>(light_list, fxhl, 3);

		SendMessage(hLowSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(hHiSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));

		SendMessage(hLowSlider, TBM_SETTICFREQ, 16, 0);
		SendMessage(hHiSlider, TBM_SETTICFREQ, 16, 0);

		sTip = CreateToolTip(hLowSlider, sTip);
		lTip = CreateToolTip(hHiSlider, lTip);

		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);

		CheckDlgButton(hDlg, IDC_SHOWAXIS, conf->hap_conf->showAxis ? BST_CHECKED : BST_UNCHECKED);

		if (eve->audio)
			eve->audio->SetDlg(hDlg);

		//if (eItem != (-1)) {
		//	ListBox_SetCurSel(light_list, eItem);
		//	SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
		//}
	}
	break;

	case WM_COMMAND:
	{
		int lbItem = (int) SendMessage(light_list, LB_GETCURSEL, 0, 0);
		int lid = (int) SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
		int fid = (int) SendMessage(freq_list, LB_GETCURSEL, 0, 0);
		map = FindMapping(lid);
		switch (LOWORD(wParam)) {
		case IDC_RADIO_OUTPUT:
		conf->hap_conf->inpType = 0;
		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);
		if (eve->audio)
			eve->audio->RestartDevice(conf->hap_conf->inpType);
		break;
		case IDC_RADIO_INPUT:
		conf->hap_conf->inpType = 1;
		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);
		if (eve->audio)
			eve->audio->RestartDevice(conf->hap_conf->inpType);
		break;
		case IDC_LIGHTS: // should reload mappings
		switch (HIWORD(wParam)) {
		case LBN_SELCHANGE:
		{
			// check in config - do we have mappings?
			if (!map) {
				haptics_map newmap;
				if (lid > 0xffff) {
					// group
					newmap.devid = 0;
					newmap.lightid = lid;
				} else {
					// light
					AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(lid);
					newmap.devid = lgh.devid;
					newmap.lightid = lgh.lightid;
				}
				newmap.colorfrom.ci = 0;
				newmap.colorto.ci = 0;
				newmap.lowcut = 0;
				newmap.hicut = 255;
				newmap.flags = 0;
				conf->hap_conf->mappings.push_back(newmap);
				//std::sort(config->mappings.begin(), config->mappings.end(), ConfigHaptics::sortMappings);
				map = FindMapping(lid);
			}
			// load freq....
			EnableWindow(freq_list, TRUE);
			SendMessage(freq_list, LB_SETSEL, FALSE, -1);
			for (int j = 0; j < map->map.size(); j++) {
				SendMessage(freq_list, LB_SETSEL, TRUE, map->map[j]);
			}
			//RedrawWindow(freq_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
			// load colors...
			RedrawButton(hDlg, IDC_BUTTON_LPC, map->colorfrom.cs.red, map->colorfrom.cs.green, map->colorfrom.cs.blue);
			RedrawButton(hDlg, IDC_BUTTON_HPC, map->colorto.cs.red, map->colorto.cs.green, map->colorto.cs.blue);
			// load cuts...
			SendMessage(hLowSlider, TBM_SETPOS, true, map->lowcut);
			SendMessage(hHiSlider, TBM_SETPOS, true, map->hicut);
			SetSlider(sTip, map->lowcut);
			SetSlider(lTip, map->hicut);
			CheckDlgButton(hDlg, IDC_GAUGE, map->flags ? BST_CHECKED : BST_UNCHECKED);
		}
		break;
		}
		break;
		case IDC_FREQ:
		{ // should update mappings list
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
			{
				if (map != NULL) {
					// add mapping
					std::vector <unsigned char>::iterator Iter = map->map.begin();
					int i = 0;
					for (i = 0; i < map->map.size(); i++)
						if (map->map[i] == fid)
							break;
						else
							Iter++;
					if (i == map->map.size())
						// new mapping, add and select
						map->map.push_back(fid);
					else
						map->map.erase(Iter);
					// remove selection
				} else {
					SendMessage(freq_list, LB_SETSEL, FALSE, -1);
				}
			} break;
			}
		} break;
		case IDC_BUTTON_LPC:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			if (map != NULL) {
				SetColor(hDlg, IDC_BUTTON_LPC, &map->colorfrom.cs.red,
						 &map->colorfrom.cs.green, &map->colorfrom.cs.blue);
			}
		} break;
		} break;
		case IDC_BUTTON_HPC:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			if (map != NULL) {
				SetColor(hDlg, IDC_BUTTON_HPC, &map->colorto.cs.red,
						 &map->colorto.cs.green, &map->colorto.cs.blue);
			}
		} break;
		} break;
		case IDC_GAUGE:
		{
			if (map)
				map->flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		} break;
		case IDC_BUTTON_REMOVE:
		{
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
			{
				if (map) {
					std::vector <haptics_map>::iterator Iter;
					for (Iter = conf->hap_conf->mappings.begin(); Iter != conf->hap_conf->mappings.end(); Iter++)
						if (Iter->devid == map->devid && Iter->lightid == map->lightid) {
							conf->hap_conf->mappings.erase(Iter);
							// clear colors...
							RedrawButton(hDlg, IDC_BUTTON_LPC, 0, 0, 0);
							RedrawButton(hDlg, IDC_BUTTON_HPC, 0, 0, 0);
							//  clear cuts....
							SendMessage(hLowSlider, TBM_SETPOS, true, 0);
							SendMessage(hHiSlider, TBM_SETPOS, true, 255);
							SetSlider(sTip, 0);
							SetSlider(lTip, 255);
							// clear selections
							SendMessage(freq_list, LB_SETSEL, FALSE, -1);
							SendMessage(light_list, LB_SETCURSEL, -1, 0);
							break;
						}
				}
			} break;
			}
		} break;
		case IDC_SHOWAXIS:
		{
			conf->hap_conf->showAxis = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
			SendMessage(hDlg, WM_PAINT, 0, 0);
		} break;
		}
	} break;
	case WM_HSCROLL:
	{
		int lbItem = (int) SendMessage(light_list, LB_GETCURSEL, 0, 0);
		int lid = (int) SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
		{
			map = FindMapping(lid);
			if (map != NULL) {
				if ((HWND) lParam == hLowSlider) {
					map->lowcut = (UCHAR) SendMessage(hLowSlider, TBM_GETPOS, 0, 0);
					SetSlider(sTip, map->lowcut);
				}
				if ((HWND) lParam == hHiSlider) {
					map->hicut = (UCHAR) SendMessage(hHiSlider, TBM_GETPOS, 0, 0);
					SetSlider(lTip, map->hicut);
				}
			}
		} break;
		}
	} break;
	case WM_DRAWITEM:
	switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
	case IDC_BUTTON_LPC: case IDC_BUTTON_HPC:
	{
		int lbItem = (int) SendMessage(light_list, LB_GETCURSEL, 0, 0);
		int lid = (int) SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
		if (lid > 0) {
			haptics_map *map = FindMapping(lid);
			if (map)
				if (((DRAWITEMSTRUCT *) lParam)->CtlID == IDC_BUTTON_LPC)
					RedrawButton(hDlg, ((DRAWITEMSTRUCT *) lParam)->CtlID, map->colorfrom.cs.red, map->colorfrom.cs.green, map->colorfrom.cs.blue);
				else
					RedrawButton(hDlg, ((DRAWITEMSTRUCT *) lParam)->CtlID, map->colorto.cs.red, map->colorto.cs.green, map->colorto.cs.blue);
		} else
			RedrawButton(hDlg, ((DRAWITEMSTRUCT *) lParam)->CtlID, 0, 0, 0);
		return 0;
	}
	case IDC_LEVELS:
	DrawFreq(hDlg, NULL);
	return 0;
	}
	break;
	case WM_CLOSE: 
	case WM_DESTROY:
	if (eve->audio)
		eve->audio->SetDlg(NULL);
	break;
	default: return false;
	}

	return TRUE;
}