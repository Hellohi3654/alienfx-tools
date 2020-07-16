#include "FXHelper.h"

FXHelper::FXHelper(int* freqp, LFXUtil::LFXUtilC* lfxUtil, ConfigHandler* conf) {
	freq = freqp;
	lfx = lfxUtil;
	config = conf;
	//lastUpdate = GetTickCount64();
	done = 0;
	stopped = 0;
	lastLights = 0;
	for (unsigned i = 0; i < 50; i++)
		updates[i].lastUpdate = 0;
};
FXHelper::~FXHelper() {
};
void FXHelper::StartFX() {
	//done = 0;
	//stopped = 0;
	lfx->Reset();
	lfx->Update();
};
void FXHelper::StopFX() {
	done = 1;
	while (!stopped)
		Sleep(100);
	lfx->Reset();
	lfx->Update();
	lfx->Release();
};

int FXHelper::Refresh(int numbars)
{
	//ULONGLONG cTime = GetTickCount64();
	//unsigned divbase = 20 / numbars;
	unsigned i = 0;
	//if (cTime - lastUpdate > 100) {
		for (i = 0; i < config->mappings.size(); i++) {
			mapping map = config->mappings[i];
			double power = 0.0;
			Colorcode from, to, fin;
			from.ci = map.colorfrom.ci; to.ci = map.colorto.ci;
			// here need to check less bars...
			for (int j = 0; j < map.map.size(); j++) {
				power += (freq[map.map[j]] > map.lowcut ? freq[map.map[j]] < map.hicut ? freq[map.map[j]] - map.lowcut : map.hicut - map.lowcut: 0 );
			}
			if (map.map.size() > 0)
				power = power / (map.map.size() * (map.hicut - map.lowcut));
			fin.cs.blue = (1 - power) * from.cs.red + power * to.cs.red;
			fin.cs.green = (1 - power) * from.cs.green + power * to.cs.green;
			fin.cs.red = (1 - power) * from.cs.blue + power * to.cs.blue;
			//it's a bug into LightFX - r and b are inverted in this call!
			fin.cs.brightness = (1 - power) * from.cs.brightness + power * to.cs.brightness;
			updates[i].color = fin;
			updates[i].devid = map.devid;
			updates[i].lightid = map.lightid;
		}
		lastLights = i;
		//if (!initdone) {
		//}
		//lfx->Update();
		//lastUpdate = GetTickCount64();
	//}
	return 0;
}

int FXHelper::UpdateLights() {
	if (done) { stopped = 1; return 1; }
	ULONGLONG cTime = GetTickCount64();
	ULONGLONG oldTime = cTime;
	unsigned uIndex = 0;
	for (unsigned j = 0; j < lastLights; j++) {
		if (oldTime > updates[j].lastUpdate) {
			oldTime = updates[j].lastUpdate;
			uIndex = j;
		}
	}
	//if (cTime - oldTime > 50) {
		lfx->SetOneLFXColor(updates[uIndex].devid, updates[uIndex].lightid, &updates[uIndex].color.ci);
		lfx->Update();
		updates[uIndex].lastUpdate = cTime;
	//}
	return 0;
}