#include "stdafx.h"
#include "stdio.h"
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "includes\injector\injector.hpp"
#include "includes\IniReader.h"

volatile int resX = 1280;
volatile int resY = 720;
volatile float aspect;

char UserDir[255];
char RenderCapsIni[255];
bool bRerouteSaveDir = false;
bool bEnableConsole = false;

int(__thiscall*ReadIni_Float)(unsigned int dis, char *section, char *key, float* value) = (int(__thiscall*)(unsigned int, char*, char*, float*))0x00527650;
char*(*GetUserDir)() = (char*(*)())0x0053A5F0;
int(__thiscall*WriteRenderCaps)(unsigned int dis) = (int(__thiscall*)(unsigned int))0x005410D0;
void InjectRes();
int InitRenderCaps();

void GetDesktopRes(volatile int* desktop_resX, volatile int* desktop_resY)
{
	HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
	MONITORINFO info = {};
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	*desktop_resX = info.rcMonitor.right - info.rcMonitor.left;
	*desktop_resY = info.rcMonitor.bottom - info.rcMonitor.top;
}

volatile float RecalculateFOV_4by3(float InFOV)
{
	float hfovRad = InFOV * M_PI / 180.0;
	float vfovRad = 2.0f*atan(tan(hfovRad*0.5) / (4.0 / 3.0));
	float newfovRad = 2.0f*atan(aspect*tan(vfovRad*0.5f));
	float vfov = (vfovRad * 180.0) / M_PI;
	float newfov = (newfovRad * 180.0) / M_PI;


	//printf("FOVCALC: hfovRad = %.2f | vfovRad = %.2f | vfov = %.2f | resultrad = %.2f | result = %.2f | aspect = %.2f\n", hfovRad, vfovRad, vfov, newfovRad, newfov, aspect);

	return newfov;
}

bool CheckForPathAbsolution(const char* input)
{
	if ((input[0] == '.') || (input[1] == '\\') || (input[1] == ':') || (input[2] == '\\'))
		return true;
	return false;
}

int __stdcall WriteRenderCaps_Hook()
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	unsigned int returnval = WriteRenderCaps(thethis);
	InitRenderCaps();
	InjectRes();

	return returnval;
}

int __stdcall ReadIni_Float_Hook(char *section, char *key, float* value)
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	int returnval = ReadIni_Float(thethis, section, key, value);
	float NewFOV = RecalculateFOV_4by3(*value);
	printf("FOVHAX: [%s] %s | READ: %.2f | WRITE: %.2f\n", section, key, *value, NewFOV);
	*value = NewFOV;
	return returnval;
}

char* GetUserDir_Hook()
{
	if (!bRerouteSaveDir)
		return GetUserDir();
	return UserDir;
}

int InitRenderCaps()
{
	if (CheckForPathAbsolution(GetUserDir_Hook()))
		sprintf(RenderCapsIni, "%s\\rendercaps.ini", GetUserDir_Hook());
	else
		sprintf(RenderCapsIni, "..\\%s\\rendercaps.ini", GetUserDir_Hook()); // due to inireader's path defaulting to the scripts folder

	// read the resolution from rendercaps.ini file directly
	CIniReader rendercaps(RenderCapsIni);
	resX = rendercaps.ReadInteger("Graphics", "Width", 0);
	resY = rendercaps.ReadInteger("Graphics", "Height", 0);

	if (!(resX || resY))
	{
		GetDesktopRes(&resX, &resY);
	}

	// equaluize the GraphicsFE with Graphics & kill res limiter
	rendercaps.WriteInteger("Graphics", "LimitResolution", 0);
	rendercaps.WriteInteger("GraphicsFE", "Width", resX);
	rendercaps.WriteInteger("GraphicsFE", "Height", resY);
	rendercaps.WriteInteger("GraphicsFE", "ScreenModeIndex", rendercaps.ReadInteger("Graphics", "ScreenModeIndex", 0));
	rendercaps.WriteInteger("GraphicsFE", "BDepth", rendercaps.ReadInteger("Graphics", "BDepth", 32));
	rendercaps.WriteInteger("GraphicsFE", "ZDepth", rendercaps.ReadInteger("Graphics", "ZDepth", 24));
	rendercaps.WriteInteger("GraphicsFE", "Stencil", rendercaps.ReadInteger("Graphics", "Stencil", 8));

	aspect = (float)resX / (float)resY;

	return 0;
}

int InitConfig()
{
	CIniReader inireader("");
	const char* InputDirString = inireader.ReadString("HP2WSFix", "SaveDir", NULL);
	bEnableConsole = inireader.ReadInteger("HP2WSFix", "EnableConsole", false);

	if (InputDirString || !strcmp(InputDirString, "0"))
	{
		bRerouteSaveDir = true;
		strncpy(UserDir, InputDirString, 255);
	}
	InitRenderCaps();

	return 0;
}

void InjectRes()
{
	injector::WriteMemory<float>(0x40C501, aspect, true);
	injector::WriteMemory<float>(0x53EB84, aspect, true);
	injector::WriteMemory<float>(0x53ED7C, aspect, true);

	// mov
	injector::WriteMemory<int>(0x541610, resX, true);
	injector::WriteMemory<int>(0x541617, resY, true);

	// push
	injector::WriteMemory<int>(0x00541489, resX, true);
	injector::WriteMemory<int>(0x00541484, resY, true);

	// push
	injector::WriteMemory<int>(0x005414A5, resX, true);
	injector::WriteMemory<int>(0x005414A0, resY, true);

	// push
	injector::WriteMemory<int>(0x005415F4, resX, true);
	injector::WriteMemory<int>(0x005415EF, resY, true);

	// cmp
	injector::WriteMemory<int>(0x004D6A39, resX, true);
	injector::WriteMemory<int>(0x004D6A42, resY, true);

	// mov
	injector::WriteMemory<int>(0x005414DD, resX, true);
	injector::WriteMemory<int>(0x005414E4, resY, true);

	// cmp
	injector::WriteMemory<int>(0x00541899, resX, true);
	injector::WriteMemory<int>(0x005418A1, resY, true);

	injector::WriteMemory<float>(0x0069D004, (float)resX, true);
	injector::WriteMemory<float>(0x0069D008, (float)resY, true);

	injector::WriteMemory<float>(0x0069D014, (float)resX, true);
	injector::WriteMemory<float>(0x0069D018, (float)resY, true);

	injector::WriteMemory<float>(0x6639F0, (float)resX, true);
	injector::WriteMemory<float>(0x6639F4, (float)resY, true);
}

int InitInjector()
{
	InjectRes();

	// dirty FOV ini read hooks (WILL GET REPLACED BY PROPER SCALING HACKS LATER)
	injector::MakeCALL(0x0040EC2F, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x0040ED5C, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x0040EE83, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x0040F624, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x0040F751, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x0040F878, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x0040FF33, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x0040FFFC, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x004100C5, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x004105E4, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x00410626, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x00410668, ReadIni_Float_Hook, true);

	// User directory hax
	injector::MakeCALL(0x00437F4D, GetUserDir_Hook, true);
	injector::MakeCALL(0x00438027, GetUserDir_Hook, true);
	injector::MakeCALL(0x446896, GetUserDir_Hook, true);
	injector::MakeCALL(0x446F50 + 0x38F, GetUserDir_Hook, true);
	injector::MakeCALL(0x446F50 + 0x3C6, GetUserDir_Hook, true);
	injector::MakeCALL(0x447F90, GetUserDir_Hook, true);
	injector::MakeCALL(0x4487CE, GetUserDir_Hook, true);
	injector::MakeCALL(0x448B80 + 0x2A, GetUserDir_Hook, true);
	injector::MakeCALL(0x44FC50 + 0x98, GetUserDir_Hook, true);
	injector::MakeCALL(0x456120 + 0x691, GetUserDir_Hook, true);
	injector::MakeCALL(0x457075, GetUserDir_Hook, true);
	injector::MakeCALL(0x457290 + 0x2DF, GetUserDir_Hook, true);
	injector::MakeCALL(0x457BB0 + 0x37F, GetUserDir_Hook, true);
	injector::MakeCALL(0x00458662, GetUserDir_Hook, true);
	injector::MakeCALL(0x458720 + 0x20, GetUserDir_Hook, true);
	injector::MakeCALL(0x458800 + 0x2E, GetUserDir_Hook, true);
	injector::MakeCALL(0x458A60 + 0x17, GetUserDir_Hook, true);
	injector::MakeCALL(0x4CF7A0 + 0x11, GetUserDir_Hook, true);
	injector::MakeCALL(0x004CFBF3, GetUserDir_Hook, true);
	injector::MakeCALL(0x4CFC10 + 0x9, GetUserDir_Hook, true);
	injector::MakeCALL(0x004CFC79, GetUserDir_Hook, true);
	injector::MakeCALL(0x4D6A4E, GetUserDir_Hook, true);
	injector::MakeCALL(0x4D9F60 + 0x2F, GetUserDir_Hook, true);
	injector::MakeCALL(0x4DA000 + 0x35, GetUserDir_Hook, true);
	injector::MakeCALL(0x4DA300 + 0x2F, GetUserDir_Hook, true);
	injector::MakeCALL(0x4DA3A0 + 0x35, GetUserDir_Hook, true);
	injector::MakeCALL(0x4DA4C0 + 0x2E, GetUserDir_Hook, true);
	injector::MakeCALL(0x4EFDA0 + 0x44, GetUserDir_Hook, true);
	injector::MakeCALL(0x4F0320 + 0x43, GetUserDir_Hook, true);
	injector::MakeCALL(0x507450 + 0x47, GetUserDir_Hook, true);
	injector::MakeCALL(0x507AB0 + 0x46, GetUserDir_Hook, true);
	injector::MakeCALL(0x5133BA, GetUserDir_Hook, true);
	injector::MakeCALL(0x513220 + 0x1FF, GetUserDir_Hook, true);
	injector::MakeCALL(0x00514341, GetUserDir_Hook, true);
	injector::MakeCALL(0x540EB0 + 0x1E, GetUserDir_Hook, true);
	injector::MakeCALL(0x5410D0 + 0x1E, GetUserDir_Hook, true);
	injector::MakeCALL(0x549D30 + 0x4A, GetUserDir_Hook, true);
	injector::MakeCALL(0x549F20 + 0x35, GetUserDir_Hook, true);

	// WriteRenderCaps
	injector::MakeCALL(0x0046F0EC, WriteRenderCaps_Hook, true);
	injector::MakeCALL(0x0046FE53, WriteRenderCaps_Hook, true);
	injector::MakeCALL(0x004D656E, WriteRenderCaps_Hook, true);

	injector::MakeJMP(0x00538140, printf, true);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		InitConfig();
		if (bEnableConsole)
		{
			AttachConsole(ATTACH_PARENT_PROCESS);
			AllocConsole();
			freopen("CON", "w", stdout);
			freopen("CON", "w", stderr);
		}
		InitInjector();
	}
	return TRUE;
}

