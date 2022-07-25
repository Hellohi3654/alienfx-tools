#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

extern AlienFan_SDK::Control* acpi;
extern ConfigFan* fan_conf;

MonHelper::MonHelper(ConfigFan* config) {
	maxTemps.resize(acpi->HowManySensors());
	senValues.resize(acpi->HowManySensors());
	fanRpm.resize(acpi->HowManyFans());
	boostRaw.resize(acpi->HowManyFans());
	boostSets.resize(acpi->HowManyFans());
	fanSleep.resize(acpi->HowManyFans());

	Start();
}

MonHelper::~MonHelper() {
	Stop();
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
		if ((oldPower = acpi->GetPower()) != fan_conf->lastProf->powerStage)
			acpi->SetPower(fan_conf->lastProf->powerStage);
		acpi->SetGPU(fan_conf->lastProf->GPUPower);
		oldGmode = acpi->GetGMode();
		if (oldGmode >= 0 && oldGmode != fan_conf->lastProf->gmode)
			acpi->SetGMode(fan_conf->lastProf->gmode);
#ifdef _DEBUG
		OutputDebugString("Mon thread start.\n");
#endif
		monThread = new ThreadHelper(CMonProc, this, 500, THREAD_PRIORITY_BELOW_NORMAL);
	}
}

void MonHelper::Stop() {
	if (monThread) {
#ifdef _DEBUG
		OutputDebugString("Mon thread stop.\n");
#endif
		delete monThread;
		monThread = NULL;
		if (oldGmode >= 0 && oldGmode != fan_conf->lastProf->gmode)
			acpi->SetGMode(oldGmode);
		if (oldPower != fan_conf->lastProf->powerStage)
			acpi->SetPower(oldPower);
		if (!oldPower)
			// reset boost
			for (int i = 0; i < acpi->fans.size(); i++)
				acpi->SetFanBoost(i, 0);
	}
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;

	// update values.....

	// temps..
	for (int i = 0; i < acpi->HowManySensors(); i++) {
		src->senValues[i] = acpi->GetTempValue(i);
		if (src->senValues[i] > src->maxTemps[i])
			src->maxTemps[i] = src->senValues[i];
	}

	// fans...
	for (int i = 0; i < acpi->HowManyFans(); i++) {
		src->boostSets[i] = 0;
		src->boostRaw[i] = acpi->GetFanBoost(i, true);
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	// boosts..
	if (!fan_conf->lastProf->powerStage) {
		// in manual mode only
		for (auto cIter = fan_conf->lastProf->fanControls.begin(); cIter < fan_conf->lastProf->fanControls.end(); cIter++) {
			for (auto fIter = cIter->fans.begin(); fIter < cIter->fans.end(); fIter++) {
				// Look for boost point for temp...
				for (int k = 1; k < fIter->points.size(); k++)
					if (src->senValues[cIter->sensorIndex] <= fIter->points[k].temp) {
						int tBoost = (fIter->points[k - 1].boost +
								((fIter->points[k].boost - fIter->points[k - 1].boost) *
								(src->senValues[cIter->sensorIndex] - fIter->points[k - 1].temp)) /
								(fIter->points[k].temp - fIter->points[k - 1].temp)) * acpi->boosts[fIter->fanIndex] / 100;
						if (tBoost > src->boostSets[fIter->fanIndex])
							src->boostSets[fIter->fanIndex] = tBoost;
						break;
					}
			}
		}
		// Now set if needed...
		for (int i = 0; i < acpi->HowManyFans(); i++)
			if (!src->fanSleep[i]) {
				// Check overboost tricks...
				if (src->boostRaw[i] < 90 && src->boostSets[i] > 100) {
					acpi->SetFanBoost(i, 100, true);
					src->fanSleep[i] = 6;
					DebugPrint(("Overboost started, locked for 3 sec (old " +to_string(src->boostRaw[i]) + ", new " + to_string(src->boostSets[i]) +")!\n").c_str());
				} else
					if (src->boostSets[i] != src->boostRaw[i] || src->boostSets[i] > 100) {
						if (src->boostRaw[i] > src->boostSets[i])
							src->boostSets[i] += 31 * (src->boostRaw[i] - src->boostSets[i]) / 32;
						acpi->SetFanBoost(i, src->boostSets[i], true);
						//DebugPrint(("Boost for fan#" + to_string(i) + " changed from " + to_string(src->boostRaw[i])
						//	+ " to " + to_string(src->boostSets[i]) + "\n").c_str());
					}
			}
			else
				src->fanSleep[i]--;
	}

}