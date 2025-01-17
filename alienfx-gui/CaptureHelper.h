#pragma once
#include "FXHelper.h"

#define GRIDSIZE 36 // 4x3 x 3

class CaptureHelper
{
public:
	CaptureHelper();
	~CaptureHelper();
	void SetCaptureScreen(int mode);
	void Start();
	void Stop();
	void Restart();
	void SetLightGridSize(int, int);
	bool needUpdate = false;
	byte *imgz;
private:
	HANDLE dwHandle = NULL;
};

