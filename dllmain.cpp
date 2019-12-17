// NFSHP2 widescreen fix
// TODO: reduce & optimize expensive strcmp's
// TODO: tidy up the code a bit more

#include "stdafx.h"
#include "stdio.h"
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <direct.h>
#include "includes\injector\injector.hpp"
#include "includes\IniReader.h"

#define TEST_XSCALE 1.6
#define TEST_YSCALE 1.2

#define FE_XPOS_ADDRESS 0x0084BC48
#define FE_YPOS_ADDRESS 0x0084BC4C

#define FE_XSCALE_ADDRESS 0x006C2DA8
#define FE_YSCALE_ADDRESS 0x006C2DAC

#define FE_FUNCTIONS_XSCALE_ADDRESS 0x006C2DB0
#define FE_FUNCTIONS_YSCALE_ADDRESS 0x006C2DB4

#define FOUR_BY_THREE_ASPECT 1.3333333333333333333333333333333
#define LETTERBOX_ASPECT 2.1052631578947368421052631578947
#define INTRO_LETTERBOX_ASPECT 2.9411764705882352941176470588235

#define FE_HOR_SCALE_DRAW_ADDR 0x0054FF7D

void InjectRes();
int InitRenderCaps();

volatile int resX = 1280;
volatile int resY = 720;
volatile float resX_43f = 960.0;
volatile float resX_600height = 1067.0;
volatile float resX_480height = 853.333334;
volatile float aspect;
volatile float aspect_diff = 0.75;
volatile float xscale_800 = 1.33375;
volatile float xscale_640 = FOUR_BY_THREE_ASPECT;

char UserDir[255];
char RenderCapsIni[255];
char* CurrentFEElement;
char* CurrentFEShapePointer;
char CurrentFEShape[255];
char* CurrentFEChild;
char* LastFEChild;
//bool bRerouteSaveDir = false;
bool bEnableConsole = false;
int FixHUD = 1;
unsigned int RenderMemorySize = 0x732000;

float FE_horscale = 1.0;
float FE_horposition = 0.0;
int OptionsMenuXsize = 0;

int(__thiscall*ReadIni_Float)(unsigned int dis, char *section, char *key, float* value) = (int(__thiscall*)(unsigned int, char*, char*, float*))0x00527650;
int(__thiscall*ReadIni_Int)(unsigned int dis, char *section, char *key, int* value) = (int(__thiscall*)(unsigned int, char*, char*, int*))0x00527560;
//char*(__stdcall*GetUserDir)() = (char*(__stdcall*)())0x0053A5F0;
int(__thiscall*WriteRenderCaps)(unsigned int dis) = (int(__thiscall*)(unsigned int))0x005410D0;
int(__thiscall*sub_59B840)(unsigned int dis, char *key, unsigned int unk1, unsigned int unk2) = (int(__thiscall*)(unsigned int, char*, unsigned int, unsigned int))0x59B840;
int(__thiscall*sub_59B6E0)(unsigned int dis, char *key, unsigned int unk1, unsigned int unk2) = (int(__thiscall*)(unsigned int, char*, unsigned int, unsigned int))0x59B6E0;
int(__thiscall*sub_5994D0)(unsigned int dis, unsigned int unk1) = (int(__thiscall*)(unsigned int, unsigned int))0x5994D0;
int(__thiscall*sub_5997B0)(unsigned int dis) = (int(__thiscall*)(unsigned int))0x5997B0;

struct UnkClass1
{
	void* vtable;
	int x;
	int y;
};

struct mBorders
{
	int unk;
	int topY;
	int topX;
	int botX;
	int botY;
};

struct bVector4
{
	float x;
	float y;
	float z;
	float w;
};

struct bMatrix4
{
	struct bVector4 v0;
	struct bVector4 v1;
	struct bVector4 v2;
	struct bVector4 v3;
};

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

int __stdcall sub_59BAE0_hook(int a2, int a3)
{
	int v3; // eax@1
	int result; // eax@2
	unsigned int thethis = 0;
	_asm mov thethis, ecx

	v3 = (*(int(__stdcall **)(int))(*(int*)thethis + 20))(a2);
	CurrentFEShapePointer = *(char**)(v3 + 4);
	strcpy(CurrentFEShape, CurrentFEShapePointer);

	if (*(int*)(v3 + 12))
	{
		sub_5994D0(a3, v3);
		result = 1;
	}
	else
	{
		sub_5997B0(a3);
		result = 0;
	}
	return result;
}

int __cdecl sub_5954A0(int a1, int a2)
{
	float v8; // [sp+1Ch] [bp+Ch]@1
	float v9; // [sp+1Ch] [bp+Ch]@1
	float v10; // [sp+1Ch] [bp+Ch]@1
	float v11; // [sp+1Ch] [bp+Ch]@1

	v8 = (double)(*(int*)(a2 + 4) + *(int*)(a2 + 16)) * *(float*)FE_FUNCTIONS_YSCALE_ADDRESS - *(float*)FE_YPOS_ADDRESS;
	v10 = (double)*(signed int *)(a2 + 4) * *(float*)FE_FUNCTIONS_YSCALE_ADDRESS - *(float*)FE_YPOS_ADDRESS;

	// the subtraction by the FE X position causes weird text bugs... removing that fixes text
	v9 = (double)(*(int*)(a2 + 8) + *(int*)(a2 + 12)) * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS;
	v11 = (double)*(signed int *)(a2 + 8) * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS;

	*(int*)(a1 + 8) = (int)v11;
	*(int*)(a1 + 12) = (int)(v9 - v11);
	*(int*)a1 = 0x65E010;
	*(int*)(a1 + 4) = (int)v10;
	*(int*)(a1 + 16) = (int)(v8 - v10);
	return a1;
}

UnkClass1* __cdecl FE_CursorPos(UnkClass1* out, UnkClass1* in)
{
	float v5;
	float v6;

	v6 = (*in).x * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS - (*(float*)FE_XPOS_ADDRESS / *(float*)FE_XSCALE_ADDRESS);
	v5 = (*in).y * *(float*)FE_FUNCTIONS_YSCALE_ADDRESS - *(float*)FE_YPOS_ADDRESS;

	(*out).vtable = (void*)0x65E1F8;
	(*out).x = (int)v6;
	(*out).y = (int)v5;
	return out;
}

int __stdcall sub_59B840_hook(char *key, mBorders* unk1, unsigned int unk2)
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	unsigned int retval1 = sub_59B840(thethis, key, (int)unk1, unk2);

	if (FixHUD > 1)
		(*unk1).botX = resX_600height;

	if (FixHUD == 1)
		*(float*)FE_XPOS_ADDRESS = FE_horposition;

	return retval1;
}

int __stdcall sub_59B6E0_hook(char *key, unsigned int unk1, unsigned int unk2) // hook for fixing specific UI elements' mBorders
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	unsigned int retval1 = sub_59B6E0(thethis, key, unk1, unk2);
	CurrentFEChild = *(char**)(unk1 + 4);

	return retval1;
}

int __stdcall sub_59B840_hook_2(char *key, mBorders* unk1, unsigned int unk2) // hook for fixing specific UI elements' mBorders
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	CurrentFEElement = *(char**)(thethis + 8);
	unsigned int retval = sub_59B840(thethis, key, (int)unk1, unk2);

	if (FixHUD > 1)
	{
		(*unk1).topX *= xscale_800;
		if ((*unk1).botX == 800)
		{
			(*unk1).botX = resX_600height;
		}
		if ((*unk1).botX == 640)
		{
			(*unk1).botX = resX_480height;
		}
		//printf("TESTVAR_X = %X\nTESTVAR_Y = %X\n", &testvarX, &testvarY);

		// SLIDER FIX START
		// sliders are a bit of their own kind, their boundary box has to increase with the screen size in order to draw properly
		if (strcmp(strrchr(CurrentFEElement, '.'), ".CmnSlider") == 0)
		{
			(*unk1).botX *= xscale_800;
			(*unk1).botX += 2;
		}
		if (strcmp(CurrentFEShape, "[PC:C_CNTRLS01.TPG.PCD] slBG") == 0)
		{
			(*unk1).botX *= xscale_800;
		}
		// SLIDER FIX END

		// fix for hud - this will have to be coded specially for every element that is broken...
		if (strcmp(CurrentFEElement, "s1.HudText") == 0)
		{
			(*unk1).topX = resX_600height - 82;
			(*unk1).topY = 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "s2.HudText") == 0)
		{
			(*unk1).topX = resX_600height - 82;
			(*unk1).topY = 19;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "s3.HudText") == 0)
		{
			(*unk1).topX = resX_600height - 82;
			(*unk1).topY = 36;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "sl1.HudText") == 0)
		{
			(*unk1).topX = resX_600height - (*unk1).botX - 98;
			(*unk1).topY = 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "sl2.HudText") == 0)
		{
			(*unk1).topX = resX_600height - (*unk1).botX - 96;
			(*unk1).topY = 19;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "sl3.HudText") == 0)
		{
			(*unk1).topX = resX_600height - (*unk1).botX - 97;
			(*unk1).topY = 36;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "p1.HudText") == 0)
		{
			(*unk1).topX = 16;
			(*unk1).topY = 0;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "p2.HudText") == 0)
		{
			(*unk1).topX = 64;
			(*unk1).topY = 14;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "score.HudText") == 0)
		{
			(*unk1).topX = 0;
			(*unk1).topY = 64;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "map.HudMap") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 473;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<TEXT>.FlashText") == 0)
		{
			if ((*unk1).topX == 0 && (*unk1).topY == 125 && (*unk1).botX == 142 && (*unk1).botY == 12)
			{
				(*unk1).topX = 0;
				(*unk1).topY = -16;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bi1.HudStaticArt") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 2;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bi2.HudStaticArt") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 21;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bi3.HudStaticArt") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 40;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bt1.HudText") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 40;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bt2.HudText") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 21;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bt3.HudText") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 2;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}


		// RVM start
		if (strcmp(CurrentFEElement, "rearview.HudRearView") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			(*unk1).topY = 4;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEShape, "[PC:H_MIRROR.TPG.PCD] edge") == 0)
		{
			if (strcmp(CurrentFEElement, "i0.GStaticImage") == 0)
			{
				(*unk1).topX = -8;
				(*unk1).topY = -3;
				//return retval;
			}
			if (strcmp(CurrentFEElement, "i0a.GStaticImage") == 0)
			{
				(*unk1).topX = 199;
				(*unk1).topY = -3;
				//return retval;
			}
		}
		// RVM end

		// tach start
		if (strcmp(CurrentFEElement, "digitach.HudTachometer") == 0)
		{
			(*unk1).topX = resX_600height - 60;
			(*unk1).topY += 20; // this will need fixing if we're gonna do 1:1 scaling...
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEShape, "[PC:H_TACH.TPG.PCD] bck1") == 0)
		{
			(*unk1).topX = -72;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "5500.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / -(32.958333333333333333333333333333);
			(*unk1).topX = -24;
			(*unk1).topY = 36;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "6000.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / -(46.529411764705882352941176470588);
			(*unk1).topX = -17;
			(*unk1).topY = 34;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "6500.GSimpleImage") == 0)
		{
			(*unk1).topX = -10;
			(*unk1).topY = 32;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "7000.GSimpleImage") == 0)
		{
			(*unk1).topX = -2;
			(*unk1).topY = 32;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "7500.GSimpleImage") == 0)
		{
			(*unk1).topX = 6;
			(*unk1).topY = 32;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "8000.GSimpleImage") == 0)
		{
			(*unk1).topX = 12;
			(*unk1).topY = 32;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "8500.GSimpleImage") == 0)
		{
			(*unk1).topX = 19;
			(*unk1).topY = 34;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "9000.GSimpleImage") == 0)
		{
			(*unk1).topX = 26;
			(*unk1).topY = 36;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "speed.HudText") == 0)
		{
			if ((*unk1).botX == 84 && (*unk1).botY == 17)
			{
				(*unk1).topX = -35;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "gear.HudText") == 0)
		{
			if ((*unk1).botX == 36 && (*unk1).botY == 16)
			{
				(*unk1).topX = 4;
				//return retval;
			}
		}
		// tach end

		// dash start
		if (strcmp(CurrentFEElement, "dash.HudDashboard") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			(*unk1).topY -= 6;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "speed.HudText") == 0)
		{
			if (!((*unk1).botX == 84 && (*unk1).botY == 17))
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "gear.HudText") == 0)
		{
			if (!((*unk1).botX == 36 && (*unk1).botY == 16))
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "speedo.HudSpeedometer") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "tach.HudTachometer") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}
		// dash end

		// EA Trax Chyron start
		if (strcmp(CurrentFEElement, "<ARTIST>.GText") == 0)
		{
			(*unk1).topX = 0;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<TITLE>.GText") == 0)
		{
			(*unk1).topX = 0;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<ALBUM>.GText") == 0)
		{
			(*unk1).topX = 0;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<BRAND>.GText") == 0)
		{
			(*unk1).topX = 0;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "b1.GImageBox") == 0)
		{
			if ((*unk1).botX == 175 && (*unk1).botY == 17)
			{
				(*unk1).topX = -2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "b2.GImageBox") == 0)
		{
			if ((*unk1).botX == 175 && (*unk1).botY == 17)
			{
				(*unk1).topX = -2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "a1.GImageBox") == 0)
		{
			if ((*unk1).botX == 176 && (*unk1).botY == 71)
			{
				(*unk1).topX = -2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "a1b.GImageBox") == 0)
		{
			if ((*unk1).botX == 76 && (*unk1).botY == 71)
			{
				(*unk1).topX = -78;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "b1b.GImageBox") == 0)
		{
			if ((*unk1).botX == 76 && (*unk1).botY == 17)
			{
				(*unk1).topX = -78;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "b2b.GImageBox") == 0)
		{
			if ((*unk1).botX == 76 && (*unk1).botY == 17)
			{
				(*unk1).topX = -78;
				//return retval;
			}
		}
		// EA Trax Chyron end

		// cuffometer
		if (strcmp(CurrentFEElement, "<CUFFOMETER>.WCuffometer") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			(*unk1).topY = (600 - (*unk1).botY) - 2;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<star0>.GStaticImage") == 0)
		{
			(*unk1).topX = 26;
			(*unk1).topY = 16;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<star1>.GStaticImage") == 0)
		{
			(*unk1).topX = 46;
			(*unk1).topY = 16;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<star2>.GStaticImage") == 0)
		{
			(*unk1).topX = 66;
			(*unk1).topY = 16;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<star3>.GStaticImage") == 0)
		{
			(*unk1).topX = 87;
			(*unk1).topY = 16;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<star4>.GStaticImage") == 0)
		{
			(*unk1).topX = 107;
			(*unk1).topY = 16;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "Reset.FlashText") == 0)
		{
			(*unk1).topX = -24;
			(*unk1).topY = -16;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<bar>.ProgressBar") == 0)
		{
			(*unk1).topX = 5;
			(*unk1).topY = 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<bar2>.ProgressBar") == 0)
		{
			(*unk1).topX = 5;
			(*unk1).topY = 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "i1.GImageBox") == 0)
		{
			if ((*unk1).botX == 75 && (*unk1).botY == 14)
			{
				(*unk1).topX = 47;
				(*unk1).topY = 20;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		// cuffometer end

		// ping display start
		if (strcmp(CurrentFEElement, "<PINGDISPLAY>.GWidget") == 0)
		{
			(*unk1).topX = 130;
			(*unk1).topY = 565;
			//return retval;
		}
		// ping display end

		// letterbox scaling
		if ((strcmp(CurrentFEElement, "boxT.GImageBox") == 0) || (strcmp(CurrentFEElement, "tttt.GImageBox") == 0 || (strcmp(CurrentFEElement, "T.GImageBox") == 0)))
		{
			(*unk1).botY = (600 - (resX_600height / LETTERBOX_ASPECT)) / 2;
			//return retval;
		}
		
		if ((strcmp(CurrentFEElement, "boxB.GImageBox") == 0) || (strcmp(CurrentFEElement, "bbbb.GImageBox") == 0 || (strcmp(CurrentFEElement, "B.GImageBox") == 0)))
		{
			(*unk1).botY = (600 - (resX_600height / LETTERBOX_ASPECT)) / 2;
			(*unk1).topY += 110 - (*unk1).botY;
			//return retval;
		}

		// title screen & load screen
		// logo
		if ((strcmp(CurrentFEElement, "LOGO.GStaticImage") == 0) || (strcmp(CurrentFEElement, "logo.GStaticImage") == 0))
		{
			(*unk1).botX = 0;
			(*unk1).botY = 0;
			//return retval;
		}
		if (strcmp(CurrentFEShape, "[PC:S_INTRO.LYR.PCD] nfsl") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "EATip.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "loadbar.GStaticImage") == 0)
		{
			(*unk1).topX = (resX_600height / 2) - 70;
			//return retval;
		}
		
		if (strcmp(CurrentFEElement, "<BAR>.GImageBox") == 0)
		{
			(*unk1).topX = (resX_600height / 2) - 64;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		// backround letterbox scale
		//if (strcmp(CurrentFEShape, "[PC:S_INTRO.LYR.PCD] back") == 0)
		//{
		//	(*unk1).topY = -((resX_600height / INTRO_LETTERBOX_ASPECT) / 2);
		//	(*unk1).botY = (resX_600height / INTRO_LETTERBOX_ASPECT) + 600;
		//	//(*unk1).topX = pow(aspect, 2) * 90;
		//	//(*unk1).topX = 435;
		//}
		// title screen end

		// titlebar fix
		if (strcmp(CurrentFEElement, "ul.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "bl.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "cl.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "ur.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "br.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "cr.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
			//return retval;
		}
		// titlebar end

		// console fix
		if (strcmp(CurrentFEElement, "!console.GStaticImage") == 0)
		{
			(*unk1).botX = resX_600height * 0.77875;
			//return retval;
		}
		// console end

		// button edges
		if (strcmp(CurrentFEElement, "L0.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "R0.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "L1.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "R1.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "!H0left.GImageBox") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 9;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "!H1left.GImageBox") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 9;
			//return retval;
		}
		// button edges end

		// Dialogbox start
		if (strcmp(CurrentFEElement, "botart.GStaticImage") == 0)
		{
			(*unk1).topX = -4;
			//return retval;
		}
		// Dialogbox end

		// Pause menu start
		if (strcmp(CurrentFEElement, "Accept.CmnPauseNav") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}
		// hud menu
		if (strcmp(CurrentFEElement, "<HUD>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 60)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<SPEEDO>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 165)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<MAP>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 95)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<REAR_VIEW>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 130)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		// gameopt menu
		if (strcmp(CurrentFEElement, "<JUMPCAM>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 60)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<360CAM>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 95)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}

		// main menu stuff
		if (strcmp(CurrentFEElement, "i_back2.GSimpleImage") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "i_logo.GSimpleImage") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<HotPursuit>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<TopCop>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<Championship>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<SingleChallenge>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<Multiplayer>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<QuickRace>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<Options>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "p5.FEParticle") == 0)
		{
			if ((*unk1).botX == 150 && (*unk1).botY == 150 && (*unk1).topY == 5)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2.5793650793650793650793650793651;
				//return retval;
			}
		}

		if (strcmp(CurrentFEElement, "p5a.FEParticle") == 0)
		{
			if ((*unk1).botX == 150 && (*unk1).botY == 150 && (*unk1).topY == 5)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 1.8258426966292134831460674157303;
				//return retval;
			}
		}

		if (strcmp(CurrentFEElement, "p3.FEParticle") == 0)
		{
			if ((*unk1).botX == 25 && (*unk1).botY == 25 && (*unk1).topY == 230)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 3.3695652173913043478260869565217;
				//return retval;
			}
		}

		if (strcmp(CurrentFEElement, "p3a.FEParticle") == 0)
		{
			if ((*unk1).botX == 25 && (*unk1).botY == 25 && (*unk1).topY == 325)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 1.4168190127970749542961608775137;
				//return retval;
			}
		}

		if (strcmp(CurrentFEElement, "Back.GButton") == 0 || strcmp(CurrentFEElement, "back.GButton") == 0 || strcmp(CurrentFEElement, "<BACK>.GButton") == 0 || strcmp(CurrentFEElement, "!Back.GText") == 0 || strcmp(CurrentFEElement, "Back.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "Next.GButton") == 0 || strcmp(CurrentFEElement, "next.GButton") == 0 || strcmp(CurrentFEElement, "<NEXT>.GButton") == 0 || strcmp(CurrentFEElement, "!Next.GText" ) == 0 || strcmp(CurrentFEElement, "Next.CmnScreenNav") == 0)
		{
			(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.47;
			//return retval;
		}

		// main menu stuff end

		// general options menu start
		// typespin control fix start
		if (strcmp(CurrentFEElement, "p1.FEParticle") == 0)
		{
			if ((*unk1).botX == 557 && (*unk1).botY == 50 && (*unk1).topY == 14)
			{
				(*unk1).botX *= xscale_800;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		// typespin control fix end

		if (strcmp(CurrentFEElement, "<VIEW>.HP2OptionsView") == 0)
		{
			(*unk1).botX *= xscale_800;
			OptionsMenuXsize = (*unk1).botX;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		// general options menu end

		// challenge setup start
		if (strcmp(CurrentFEElement, "<VIEW>.HP2SingleChallengeView") == 0)
		{
			(*unk1).botX *= xscale_800;
			//return retval;
		}
		// challenge setup end

		// tournament status start
		if (strcmp(CurrentFEElement, "<RACE>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<LIST>.GListBox") == 0)
		{
			if ((*unk1).botY == 162 && (*unk1).topY == 305)
			{
				(*unk1).topX = (resX_600height - 640) / 2;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "tx02.GText") == 0)
		{
			if ((*unk1).botX == 185 && (*unk1).botY == 20 && (*unk1).topY == 283)
			{
				(*unk1).topX = (resX_600height / 2) - 288;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "tx03.GText") == 0)
		{
			if ((*unk1).botX == 106 && (*unk1).botY == 20 && (*unk1).topY == 283)
			{
				(*unk1).topX = (resX_600height / 2) - 103;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "tx04.GText") == 0)
		{
			if ((*unk1).botX == 105 && (*unk1).botY == 20 && (*unk1).topY == 283)
			{
				(*unk1).topX = (resX_600height / 2) + 4;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "tx05.GText") == 0)
		{
			if ((*unk1).botX == 106 && (*unk1).botY == 20 && (*unk1).topY == 283)
			{
				(*unk1).topX = (resX_600height / 2) + 106;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "tx06.GText") == 0)
		{
			if ((*unk1).botX == 102 && (*unk1).botY == 20 && (*unk1).topY == 283)
			{
				(*unk1).topX = (resX_600height / 2) + 217;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "tx01b.GText") == 0)
		{
			if ((*unk1).botX == 275 && (*unk1).botY == 20 && (*unk1).topY == 249)
			{
				(*unk1).topX = (resX_600height - 165) / 2;
				(*unk1).topY = 470; // (600 - 12) / 2 // this will have to get changed if 1:1 scale will be done...
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "B1.GImageBox") == 0)
		{
			if ((*unk1).botX == 34 && (*unk1).botY == 160 && (*unk1).topY == 0)
			{
				(*unk1).topX = (resX_480height / 2) - 332;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "B2.GImageBox") == 0)
		{
			if ((*unk1).botX == 105 && (*unk1).botY == 160 && (*unk1).topY == 0)
			{
				(*unk1).topX = (resX_480height / 2) - 100;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "B3.GImageBox") == 0)
		{
			if ((*unk1).botX == 105 && (*unk1).botY == 160 && (*unk1).topY == 0)
			{
				(*unk1).topX = (resX_480height / 2) + 109;
				//return retval;
			}
		}
		// tournament status end

		// tournament trax start
		if (strcmp(CurrentFEElement, "TournTrax.GWidget") == 0)
		{
			if ((*unk1).botX == 590 && (*unk1).botY == 131 && (*unk1).topY == 360)
			{
				(*unk1).topX = (resX_600height / 2) - 295;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "H0.GText") == 0)
		{
			if ((*unk1).botX == 36 && (*unk1).botY == 17 && (*unk1).topY == -17)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "H1.GText") == 0)
		{
			if ((*unk1).botX == 211 && (*unk1).botY == 17 && (*unk1).topY == -17)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "H2.GText") == 0)
		{
			if ((*unk1).botX == 219 && (*unk1).botY == 17 && (*unk1).topY == -17)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "H3.GText") == 0)
		{
			if ((*unk1).botX == 113 && (*unk1).botY == 17 && (*unk1).topY == -17)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "C2.GImageBox") == 0)
		{
			if ((*unk1).botX == 221 && (*unk1).botY == 130 && (*unk1).topY == 1)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
				
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		// tournament trax end

		// controller options start
		if (strcmp(CurrentFEElement, "B1.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 1)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "B2.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 41)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "B3.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 81)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "B4.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 121)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "C1.GImageBox") == 0)
		{
			if ((*unk1).botX == 268 && (*unk1).botY == 160 && (*unk1).topY == 1)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "C2.GImageBox") == 0)
		{
			if ((*unk1).botX == 226 && (*unk1).botY == 160 && (*unk1).topY == 1)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "A1.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 160 && (*unk1).topY == 1)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "D1.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == -19)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "H1.GText") == 0)
		{
			if ((*unk1).botX == 264 && (*unk1).botY == 12 && (*unk1).topY == -16)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "H2.GText") == 0)
		{
			if ((*unk1).botX == 208 && (*unk1).botY == 12 && (*unk1).topY == -16)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "!h0L.GImageBox") == 0)
		{
			if ((*unk1).botX == 240 && (*unk1).botY == 2 && (*unk1).topY == -20)
			{
				(*unk1).botX *= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "!h0R.GImageBox") == 0)
		{
			if ((*unk1).botX == 240 && (*unk1).botY == 2 && (*unk1).topY == -20)
			{
				(*unk1).botX *= xscale_800;
				//return retval;
			}
		}

		// ffb & deadzone start
		if (strcmp(CurrentFEElement, "STICK.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "ROADFX.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "COLLISION.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "ENGINE.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "GRIP.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_0>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_1>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_2>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_3>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_4>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_5>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_6>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<AXIS_7>.CmnSlider") == 0)
		{
			(*unk1).topX /= xscale_800;
			(*unk1).topX -= (*unk1).botX - ((*unk1).botX / xscale_800);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<OK>.GButton") == 0)
		{
			if ((*unk1).botX == 106 && (*unk1).botY == 18 && ((*unk1).topY == 210 || (*unk1).topY == 300))
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<CANCEL>.GButton") == 0)
		{
			if ((*unk1).botX == 101 && (*unk1).botY == 18 && ((*unk1).topY == 240 || (*unk1).topY == 330))
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		// ffb & deadzone end

		// controller options end

		// credits screen start
		if (strcmp(CurrentFEElement, "!H0left.GImageBox") == 0)
		{
			(*unk1).botX *= xscale_800;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "!H1left.GImageBox") == 0)
		{
			(*unk1).botX *= xscale_800;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		// credits screen end

		// driver profile start
		if (OptionsMenuXsize)
		{
			if (strcmp(CurrentFEElement, "<LOAD>.GButton") == 0)
			{
				if ((*unk1).botX == 222 && (*unk1).botY == 16 && (*unk1).topY == 210)
				{
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
					//return retval;
				}
			}
			if (strcmp(CurrentFEElement, "<DELETE>.GButton") == 0)
			{
				if ((*unk1).botX == 222 && (*unk1).botY == 16 && (*unk1).topY == 240)
				{
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
					//return retval;
				}
			}
			if (strcmp(CurrentFEElement, "<NEW>.GButton") == 0)
			{
				if ((*unk1).botX == 222 && (*unk1).botY == 16 && (*unk1).topY == 270)
				{
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
					//return retval;
				}
			}
			if (strcmp(CurrentFEElement, "<SAVE>.GButton") == 0)
			{
				if ((*unk1).botX == 222 && (*unk1).botY == 16 && (*unk1).topY == 300)
				{
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
					//return retval;
				}
				//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
				//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			}
			if (strcmp(CurrentFEElement, "<DPL>.ToggleButton") == 0)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 15;
				//return retval;
			}
			if (strcmp(CurrentFEElement, "<DSTATS>.GListBox") == 0)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
				//return retval;
			}
			if (strcmp(CurrentFEElement, "<CURRP>.GText") == 0)
			{
				(*unk1).topX = (OptionsMenuXsize - 143) / 2 - 143;
				//return retval;
				//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
				//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			}
		}
		// driver profile end

		// car select
		if (strcmp(CurrentFEElement, "<CAR>.CarWidget") == 0)
		{
			(*unk1).topX = resX / 21.33334;
			if ((resX / 1280.0) < 2.0) // not sure when it's negative exactly
				(*unk1).topX = -(*unk1).topX;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<Showcase>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "<CAR_IL>.HPImgList") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<TRANY>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 182 && (*unk1).botY == 21 && (*unk1).topY == 241)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2 + 78;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<btl>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 45 && (*unk1).botY == 73 && (*unk1).topY == 42)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<btr>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 45 && (*unk1).botY == 73 && (*unk1).topY == 42)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "p6.FEParticle") == 0)
		{
			if ((*unk1).botX == 128 && (*unk1).botY == 128 && (*unk1).topY == -6)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "p7.FEParticle") == 0)
		{
			if ((*unk1).botX == 128 && (*unk1).botY == 128 && (*unk1).topY == -6)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}

		// car select end


		// track select
		if (strcmp(CurrentFEElement, "<Descrip>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<tdir>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<lock>.GStaticImage") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<Cost>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		//if (strcmp(CurrentFEElement, "TRCK.ActorWidget") == 0)
		//{
		//	(*unk1).botX = 800;
		//}
		if (strcmp(CurrentFEElement, "<bup>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 36 && (*unk1).botY == 22 && (*unk1).topY == -3)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<bdn>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 35 && (*unk1).botY == 22 && (*unk1).topY == 68)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}

		if (strcmp(CurrentFEElement, "p1.FEParticle") == 0)
		{
			if ((*unk1).botX == 25 && (*unk1).botY == 25 && (*unk1).topY == 33)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "p2.FEParticle") == 0)
		{
			if ((*unk1).botX == 25 && (*unk1).botY == 25 && (*unk1).topY == 33)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<Dir>.GAnimImage") == 0)
		{
			if ((*unk1).botX == 45 && (*unk1).botY == 45 && (*unk1).topY == 22)
			{
				(*unk1).topX /= xscale_800;
				//return retval;
			}
		}

		// track select end

		// event tree
		if (strcmp(CurrentFEElement, "<EVENT_ID>.GText") == 0)
		{
			//(*unk1).botX *= xscale_800;
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<EVENT_OBJECTIVE>.GText") == 0)
		{
			(*unk1).botX *= xscale_800;
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<EVENT_STATUS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height / 2) + 25;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "ET1a.GText") == 0)
		{
			(*unk1).topX = (resX_600height / 2) - 130;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "ET7.GText") == 0)
		{
			(*unk1).topX = (resX_600height / 2) - 273;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<EVENT_CAR>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<EVENT_OPPONENTS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<EVENT_LAPS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<EVENT_TRACKS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<REWARD>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<OK>.GButton") == 0)
		{
			if ((*unk1).botX == 192 && (*unk1).botY == 17 && (*unk1).topY == 350)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<CANCEL>.GButton") == 0)
		{
			if ((*unk1).botX == 190 && (*unk1).botY == 17 && (*unk1).topY == 380)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
			}
		}
		// event tree end

		// starting/ending grid
		if (strcmp(CurrentFEElement, "t0.GText") == 0)
		{
			if ((*unk1).botX == 195 && (*unk1).botY == 45 && (*unk1).topY == 65)
			{
				(*unk1).topX = 8;
				(*unk1).topY = 8;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "t0.FlashText") == 0)
		{
			if ((*unk1).botX == 191 && (*unk1).botY == 35 && (*unk1).topY == 40)
			{
				(*unk1).topX = 8;
				(*unk1).topY = 8;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "GRID.GWidget") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<StartRace>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "field.GWidget") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<LIST>.GListBox") == 0)
		{
			if ((*unk1).botX == 422 && (*unk1).botY == 160)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "<RESULTS>.GListBox") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<TRACK>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "titlebar.GImageBox") == 0)
		{
			if ((*unk1).topX)
			{
				if ((*unk1).botX == 463 && (*unk1).botY == 40)
				{
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
					//return retval;
				}
				if ((*unk1).botX == 349 && (*unk1).botY == 30)
				{
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 57;
					//return retval;
				}
				if ((*unk1).botX == 464 && (*unk1).botY == 20)
				{
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
					//return retval;
				}
			}
		}
		if (strcmp(CurrentFEElement, "B1.GButton") == 0)
		{
			if ((*unk1).botX == 222 && (*unk1).botY == 18 && (*unk1).topY == 250)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "B2.GButton") == 0)
		{
			if ((*unk1).botX == 222 && (*unk1).botY == 18 && (*unk1).topY == 290)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "B3.GButton") == 0)
		{
			if ((*unk1).botX == 222 && (*unk1).botY == 18 && (*unk1).topY == 330)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "title.GText") == 0)
		{
			if ((*unk1).botX == 464 && (*unk1).botY == 30)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
				//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
				//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			}
		}
		if (strcmp(CurrentFEElement, "h2.GText") == 0)
		{
			if ((*unk1).botX == 176 && (*unk1).botY == 20)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) - 98;
				//return retval;
			}
			if ((*unk1).botX == 164 && (*unk1).botY == 18)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) - 119;
				//return retval;
			}
		}
		if (strcmp(CurrentFEElement, "h3.GText") == 0)
		{
			if ((*unk1).botX == 221 && (*unk1).botY == 20)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) + 101;
				//return retval;
			}
			if ((*unk1).botX == 166 && (*unk1).botY == 18)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) + 47;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "h4.GText") == 0)
		{
			if ((*unk1).botX == 110 && (*unk1).botY == 18)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) + 166;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<COLUMN0>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "<COLUMN1>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<COLUMN2>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<COLUMN3>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<COLUMN4>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}
		if (strcmp(CurrentFEElement, "<COLUMN5>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "c2.GImageBox") == 0)
		{
			if ((*unk1).botX == 223 && (*unk1).botY == 160)
			{
				(*unk1).topX = 200;
				//return retval;
			}
			if ((*unk1).botX == 167 && (*unk1).botY == 160)
			{
				(*unk1).topX = 188;
				//return retval;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "<TEXT>.GText") == 0)
		{
			if ((*unk1).botX == 464 && (*unk1).botY == 63 && (*unk1).topY == 253)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//return retval;
			}
		}
		// starting/ending grid end

		// race end stuff start
		if (strcmp(CurrentFEElement, "02Points.GWidget") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//return retval;
		}
		// race end stuff end

		// misc. stuff
		if (strcmp(CurrentFEElement, "BNK1.GText") == 0)
		{
			(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.63;
			//return retval;
		}

		if (strcmp(CurrentFEElement, "tm.GText") == 0)
		{
			(*unk1).topX = ((resX_600height) / 2) + 196;
			//return retval;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		// misc. stuff end

		CurrentFEShape[0] = 0;
	}
	return retval;
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
	//if (!bRerouteSaveDir)
	//	return GetUserDir();
	return UserDir;
}

int InitRenderCaps()
{
	if (CheckForPathAbsolution(GetUserDir_Hook()))
		sprintf(RenderCapsIni, "%srendercaps.ini", GetUserDir_Hook());
	else
		sprintf(RenderCapsIni, "..\\%srendercaps.ini", GetUserDir_Hook()); // due to inireader's path defaulting to the scripts folder

	// read the resolution from rendercaps.ini file directly
	CIniReader rendercaps(RenderCapsIni);
	resX = rendercaps.ReadInteger("Graphics", "Width", 0);
	resY = rendercaps.ReadInteger("Graphics", "Height", 0);

	if (!(resX || resY))
	{
		GetDesktopRes(&resX, &resY);
	}

	// TEMPORARY HUD FIX
	if (FixHUD)
	{
		resX_43f = ((float)resY * (4.0f / 3.0f));

		FE_horscale = resX_43f / 800.0;
		FE_horposition = (resX - resX_43f) / 2;

		*(float*)FE_XSCALE_ADDRESS = FE_horscale;
		if (FixHUD == 1)
			*(float*)FE_XPOS_ADDRESS = FE_horposition;

		// fix function scaling...
		*(float*)FE_FUNCTIONS_YSCALE_ADDRESS = 1 / (resY / 600.0);
		*(float*)FE_FUNCTIONS_XSCALE_ADDRESS = *(float*)FE_FUNCTIONS_YSCALE_ADDRESS;
	}

	// equalize the GraphicsFE with Graphics & kill res limiter
	rendercaps.WriteInteger("Graphics", "LimitResolution", 0);
	rendercaps.WriteInteger("GraphicsFE", "Width", resX);
	rendercaps.WriteInteger("GraphicsFE", "Height", resY);
	rendercaps.WriteInteger("GraphicsFE", "ScreenModeIndex", rendercaps.ReadInteger("Graphics", "ScreenModeIndex", 0));
	rendercaps.WriteInteger("GraphicsFE", "BDepth", rendercaps.ReadInteger("Graphics", "BDepth", 32));
	rendercaps.WriteInteger("GraphicsFE", "ZDepth", rendercaps.ReadInteger("Graphics", "ZDepth", 24));
	rendercaps.WriteInteger("GraphicsFE", "Stencil", rendercaps.ReadInteger("Graphics", "Stencil", 8));

	aspect = (float)resX / (float)resY;
	aspect_diff = (float)FOUR_BY_THREE_ASPECT / aspect;

	resX_600height = aspect * 600;
	resX_480height = aspect * 480;
	xscale_800 = resX_600height / 800;
	xscale_640 = resX_480height / 640;

	return 0;
}

int InitConfig()
{
	CIniReader inireader("");
	const char* InputDirString = inireader.ReadString("HP2WSFix", "SaveDir", "save");
	bEnableConsole = inireader.ReadInteger("HP2WSFix", "EnableConsole", 0);
	FixHUD = inireader.ReadInteger("HP2WSFix", "FixHUD", 1);
	RenderMemorySize = inireader.ReadInteger("HP2WSFix", "RenderMemorySize", 0x732000);

	//if (InputDirString && (InputDirString[0] != '0')) // 09/2019 - BROKEN FEATURE - game nulls the pointer at 83C024 because reasons..
	//{
		//bRerouteSaveDir = true;
		//strncpy(UserDir, InputDirString, 255);

	if (CheckForPathAbsolution(InputDirString))
		sprintf(UserDir, "%s", InputDirString);
	else
		sprintf(UserDir, "..\\%s", InputDirString);
	
	_mkdir(UserDir);
	sprintf(UserDir, "%s\\", InputDirString);
	//strncpy(UserDir, InputDirString, 255);

	//}

	InitRenderCaps();

	return 0;
}

void InjectRes()
{
	injector::WriteMemory<float>(0x40C501, aspect, true);
	injector::WriteMemory<float>(0x53EB84, aspect, true);
	injector::WriteMemory<float>(0x53ED7C, aspect, true);
	// gui
	injector::WriteMemory<float>(0x0065D900, aspect, true);

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

	// METHOD 2 - FIX Y FE SCALING BY CHANGING SCALING FACTOR
	injector::WriteMemory<float>(0x00445BE3, aspect_diff, true); // FE 3D map actor Y scale
	injector::WriteMemory<float>(0x0044D59D, aspect_diff, true); // FE car actor Y scale
	injector::WriteMemory<float>(0x0049C3F3, aspect_diff, true); // FE 3D event tree actor Y scale

	// GUI hax
	if (FixHUD)
	{
		injector::WriteMemory<float>(0x45A8EC, (float)resX, true);
		injector::WriteMemory<float>(0x45A8F3, (float)resY, true);
	}
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
	injector::MakeCALL(0x00411092, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x004110EA, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x00411142, ReadIni_Float_Hook, true);
	injector::MakeCALL(0x00410D9B, ReadIni_Float_Hook, true);

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

	// "Hi-Poly" fixes

	// an actual attempt at a hi-poly fix
	injector::WriteMemory<int>(0x0053BA84, RenderMemorySize, true);

	// GUI sub_595440
	if (FixHUD)
	{
		injector::MakeCALL(0x0045A8DC, sub_59B840_hook, true);

		if (FixHUD > 1)
		{
			injector::MakeCALL(0x0059410A, sub_59B840_hook_2, true);
			injector::MakeCALL(0x00593FEE, sub_59B6E0_hook, true);
			injector::MakeCALL(0x00596F6A, sub_59BAE0_hook, true);
			injector::MakeCALL(0x005980D8, sub_59BAE0_hook, true);

			// cuffometer position fix - disable writes to its mBounds
			// X
			injector::MakeNOP(0x453C95, 3, true);
			injector::MakeNOP(0x453C16, 3, true);
			// Y
			injector::MakeNOP(0x453C1F, 3, true);
			injector::MakeNOP(0x453C98, 3, true);
		}
	injector::MakeCALL(0x00462FDF, sub_5954A0, true);
	injector::MakeCALL(0x00463501, sub_5954A0, true);

	// Y scale
	//	injector::MakeNOP(0x00595318, 6, true);

	// X scale
		injector::MakeNOP(0x00595304, 6, true);

	// Y functions scale
		injector::MakeNOP(0x00595351, 6, true);

	// X functions scale
		injector::MakeNOP(0x0059533F, 6, true);
		injector::MakeCALL(0x005A15A4, FE_CursorPos, true);

	// Y pos
	//	injector::MakeNOP(0x0059532D, 6, true);

	// X pos
		injector::MakeNOP(0x00595324, 6, true);

	}
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
			freopen("CON", "r", stdin);
			freopen("CON", "w", stdout);
			freopen("CON", "w", stderr);
		}
		InitInjector();
	}
	return TRUE;
}

