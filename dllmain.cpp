﻿// NFSHP2 widescreen fix
// WARNING: dirty, uncleaned code ahead from testing, THIS WILL BE CLEANED AT A LATER POINT
// Currently I have some test variables and test function hooks in the code commented out for testing
// STARE AT THIS AT YOUR OWN RISK

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

volatile int resX = 1280;
volatile int resY = 720;
volatile float resX_43f = 960.0;
volatile float resX_600height = 1067.0;
volatile float aspect;
volatile float aspect_diff = 0.75;
volatile float xscale_800 = 1.33375;


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
float FE_fonthorscale = 1.0;
float FE_fontverscale = 1.0;
float FE_verscale = 1.0;
float FE_horposition = 0.0;
float FE_verposition = 0.0;
int OptionsMenuXsize = 0;

float testvarX = 1.0;
float testvarY = 1.0;
float testvarZ = 1.0;
float testvarW = 1.0;
//float testvar = 1.0;


int(__thiscall*ReadIni_Float)(unsigned int dis, char *section, char *key, float* value) = (int(__thiscall*)(unsigned int, char*, char*, float*))0x00527650;
int(__thiscall*ReadIni_Int)(unsigned int dis, char *section, char *key, int* value) = (int(__thiscall*)(unsigned int, char*, char*, int*))0x00527560;
//char*(__stdcall*GetUserDir)() = (char*(__stdcall*)())0x0053A5F0;
int(__thiscall*WriteRenderCaps)(unsigned int dis) = (int(__thiscall*)(unsigned int))0x005410D0;
int(__thiscall*sub_59B840)(unsigned int dis, char *key, unsigned int unk1, unsigned int unk2) = (int(__thiscall*)(unsigned int, char*, unsigned int, unsigned int))0x59B840;
int(__thiscall*sub_463090)(unsigned int dis, int unk1, int unk2, int unk3, int unk4, int unk5) = (int(__thiscall*)(unsigned int, int, int, int, int, int))0x463090;
int(__thiscall*sub_469890)(unsigned int dis, int unk1, int unk2, int unk3, int unk4, int unk5, int unk6, int unk7) = (int(__thiscall*)(unsigned int, int, int, int, int, int, int, int))0x469890;
int(__thiscall*sub_463920)(unsigned int dis, int unk1, int unk2, int unk3, int unk4, int unk5, int unk6, int unk7) = (int(__thiscall*)(unsigned int, int, int, int, int, int, int, int))0x463920;
int(__thiscall*sub_59B6E0)(unsigned int dis, char *key, unsigned int unk1, unsigned int unk2) = (int(__thiscall*)(unsigned int, char*, unsigned int, unsigned int))0x59B6E0;
int(__thiscall*sub_5994D0)(unsigned int dis, unsigned int unk1) = (int(__thiscall*)(unsigned int, unsigned int))0x5994D0;
int(__thiscall*sub_5997B0)(unsigned int dis) = (int(__thiscall*)(unsigned int))0x5997B0;


void InjectRes();
int InitRenderCaps();



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

int __stdcall sscanf_hook(const char* str, const char* Format, ...)
{
	va_list ArgList;
	int Result = 0;

	int topX = 0;
	int topY = 0;
	int botX = 0;
	int botY = 0;

	__crt_va_start(ArgList, Format);
	Result = vsscanf(str, Format, ArgList);
	__crt_va_end(ArgList);

	__crt_va_start(ArgList, Format);
	topX = __crt_va_arg(ArgList, int);
	topY = __crt_va_arg(ArgList, int);
	botX = __crt_va_arg(ArgList, int);
	botY = __crt_va_arg(ArgList, int);
	__crt_va_end(ArgList);

	//*(int*)topX += testvarX;
	//if (*(int*)botX == 800)
	//	*(int*)botX = resX;
	//*(int*)botX *= TEST_XSCALE;

	printf("GUI.mBorders (sscanf): [ %d , %d ] %d , %d\n", *(int*)topX, *(int*)topY, *(int*)botX, *(int*)botY);


	return Result;
}

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

int __stdcall sub_463920_hook(int unk1, UnkClass1* unk2, UnkClass1* unk3, UnkClass1* unk4, UnkClass1* unk5, UnkClass1* unk6, int unk7) // animated objects
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx

	//printf("Unk1: x: %d y: %d\n", (*unk2).X, (*unk2).Y);

	// X
	//(*unk2).X = (int)((float)((*unk2).X) * TEST_XSCALE);
	//(*unk2).Y = (int)((float)((*unk2).Y) * TEST_YSCALE);
	//
	//// Y
	//(*unk3).X = (int)((float)((*unk3).X) * TEST_XSCALE);
	//(*unk3).Y = (int)((float)((*unk3).Y) * TEST_YSCALE);
	//
	//// Z
	//(*unk4).X = (int)((float)((*unk4).X) * TEST_XSCALE);
	//(*unk4).Y = (int)((float)((*unk4).Y) * TEST_YSCALE);
	//
	//// W
	//(*unk5).X = (int)((float)((*unk5).X) * TEST_XSCALE);
	//(*unk5).Y = (int)((float)((*unk5).Y) * TEST_YSCALE);
	//
	//(*unk6).X = (int)((float)((*unk6).X) * TEST_XSCALE);
	//(*unk6).Y = (int)((float)((*unk6).Y) * TEST_YSCALE);

	//printf("sub_463920 ( this: %x %x %x %x %x %x %x %x ) \n", thethis, unk1, unk2, unk3, unk4, unk5, unk6, unk7);
	//_asm int 3

	return sub_463920(thethis, unk1, (int)unk2, (int)unk3, (int)unk4, (int)unk5, (int)unk6, unk7);
}

int __cdecl sub_5954A0(int a1, int a2)
{
	float v8; // [sp+1Ch] [bp+Ch]@1
	float v9; // [sp+1Ch] [bp+Ch]@1
	float v10; // [sp+1Ch] [bp+Ch]@1
	float v11; // [sp+1Ch] [bp+Ch]@1

	//float X_scale = testvarX;
	//float X_pos = testvarY;

	//float X_scale = *(float*)FE_FUNCTIONS_XSCALE_ADDRESS;
	//float X_pos = *(float*)FE_XPOS_ADDRESS;

	v8 = (double)(*(int*)(a2 + 4) + *(int*)(a2 + 16)) * *(float*)FE_FUNCTIONS_YSCALE_ADDRESS - *(float*)FE_YPOS_ADDRESS;
	v10 = (double)*(signed int *)(a2 + 4) * *(float*)FE_FUNCTIONS_YSCALE_ADDRESS - *(float*)FE_YPOS_ADDRESS;

	//v9 = (double)(*(int*)(a2 + 8) + *(int*)(a2 + 12)) * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS - *(float*)FE_XPOS_ADDRESS;
	//v11 = (double)*(signed int *)(a2 + 8) * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS - *(float*)FE_XPOS_ADDRESS;

	//v9 = (double)(*(int*)(a2 + 8) + *(int*)(a2 + 12)) * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS - (*(float*)FE_XPOS_ADDRESS / *(float*)FE_XSCALE_ADDRESS);
	//v11 = (double)*(signed int *)(a2 + 8) * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS - (*(float*)FE_XPOS_ADDRESS / *(float*)FE_XSCALE_ADDRESS);

	//v9 = (double)(*(int*)(a2 + 8) + *(int*)(a2 + 12)) * X_scale - X_pos;
	//v11 = (double)*(signed int *)(a2 + 8) * X_scale - X_pos;

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

	//printf("sub_595550 X: %d | Y: %d | TESTVAR_X: %X | TESTVAR_Y: %X\n", (*in).x, (*in).y, &testvarX, &testvarY);

	//v6 = (*in).x/* * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS */- 480.0/* - *(float*)FE_XPOS_ADDRESS*/;

	// VARIANT 1 - move the functions towards the centre (without scaling)
	//v6 = (*in).x - ((resX - 800.0) / 2.0);

	// VARIANT 2 - scale the functions across the screenspace (kinda broken but works mostly)
	// old
	//v6 = (*in).x * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS - (*(float*)FE_XPOS_ADDRESS - ((*(float*)FE_XPOS_ADDRESS / 2) - 32.0));
	// new - seems to work flawlessly
	v6 = (*in).x * *(float*)FE_FUNCTIONS_XSCALE_ADDRESS - (*(float*)FE_XPOS_ADDRESS / *(float*)FE_XSCALE_ADDRESS);

	v5 = (*in).y * *(float*)FE_FUNCTIONS_YSCALE_ADDRESS - *(float*)FE_YPOS_ADDRESS;

	(*out).vtable = (void*)0x65E1F8;
	(*out).x = (int)v6;
	(*out).y = (int)v5;
	return out;
}

int __stdcall sub_469890_hook(int unk1, int unk2, int unk3, int unk4, int unk5, int unk6, int unk7)
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	//printf("sub_469890 ( this: %x %x %x %x %x %x %x %x ) \n", thethis, unk1, unk2, unk3, unk4, unk5, unk6, unk7);
	//_asm int 3

	return sub_469890(thethis, unk1, unk2, unk3, unk4, unk5, unk6, unk7);
}

int __cdecl sub_54B0D0(int a1, float a2, float a3, float a4, float a5)
{
	*(float *)a1 = a2;
	*(float *)(a1 + 4) = a3;
	*(float *)(a1 + 8) = a4;
	*(float *)(a1 + 12) = a5;
	return a1;
}

int __cdecl sub_46B4E0(int a1, float a2, float a3, float a4, float a5)
{
	//printf("sub_46B4E0 ( %x %.2f %.2f %.2f %.2f ) \n", a1, a2, a3, a4, a5);
	//printf("TESTVAR_X: %X\nTESTVAR_Y: %X\nTESTVAR_Z: %X\nTESTVAR_W: %X\n", &testvarX, &testvarY, &testvarZ, &testvarW);
	//a2 *= testvarX;
	//a3 *= testvarY;
	//a4 *= testvarZ;
	//a5 *= testvarW;

	*(float*)a1 = a2;
	*(float*)(a1 + 4) = a3;
	*(float*)(a1 + 8) = a4;
	*(float*)(a1 + 12) = a5;
	return a1;
}

int scalecounter = 0;

bVector4* __cdecl sub_46B470(bVector4 *vector, float X, float Y, float UnkX, float UnkY, float BoundX, float BoundY)
{
	float v8; // st7@1
	float v10; // st7@1

	float X_out = X * (1.0f / resX * (resY / BoundX)) * 2.0f;
	float Y_out = Y * TEST_YSCALE;
	float tst1;
	float tst2;

	//UnkX = 0;
	//UnkY = 0;


	//X *= TEST_XSCALE;
	//Y *= TEST_YSCALE;

	printf("sub_46B470 ( %x X: %.2f Y: %.2f UnkX: %.2f UnkY: %.2f BoundX: %.2f BoundY: %.2f ) \n", vector, X, Y, UnkX, UnkY, BoundX, BoundY);
	//_asm int 3
	
	//getchar();
	//UnkX = BoundX;
	//UnkY = BoundY;

	/*if ((BoundX == (float)resX))
	{
		UnkX -= 240;
	}
	if ((BoundY == (float)resY))
	{
		UnkY -= 60;
	}*/

	if ((BoundX == 800.0))
	{
		//X -= FE_horposition;
		//_asm int 3
		//BoundX = resX;
		//UnkX -= 520;
		//X += testvar;
	}
	
	if ((BoundY == 600.0))
	{
		//Y -= FE_verposition;
		//BoundY = resY;
		//UnkY -= testvarY;
		//X += testvar;
	}

	//if ((BoundX == (float)resX) && (BoundY == (float)resY))
	//{
	//	//X *= TEST_XSCALE;
	//	//Y *= TEST_YSCALE;
	//	//UnkX *= TEST_XSCALE;
	//	//UnkX += testvar;
	//	//Y += 60;
	//	UnkX -= 240;
	//	UnkY -= 60;
	//}

	/*if (!((BoundX == 800.0) && (BoundY == 600.0)))
	{
		X *= TEST_XSCALE;
		Y *= TEST_YSCALE;
	}
	else
	{*/
		//UnkX = 0;
		//UnkY = 0;
	//}
	//else
	//{*/
		//X += 240;
		//Y += 60;
	//}

	//X += ((float)resX - BoundX) / 2;
	//X *= TEST_XSCALE;
	//Y *= TEST_YSCALE;

	//printf("TESTVAR_X: %X\nTESTVAR_Y: %X\n", &testvarX, &testvarY);


	v8 = (X_out - (BoundX * 0.5 + UnkX)) / BoundX; // horizontal
	v10 = (Y_out - (BoundY * 0.5 + UnkY)) / BoundY; // vertical


	(*vector).x = v8 * 2;
	(*vector).y = -v10 * 2;
	(*vector).z = -1.0;
	(*vector).w = 1.0;

	return vector;
}


int __cdecl sub_595390_3Dhook(int a1, int a2) // NOT SURE IF ASPECT RATIO IS CORRECT IN THIS ONE!!!
{
  float v8; // [sp+1Ch] [bp+Ch]@1
  float v9; // [sp+1Ch] [bp+Ch]@1
  float v10; // [sp+1Ch] [bp+Ch]@1
  float v11; // [sp+1Ch] [bp+Ch]@1

  // THIS NEEDS FIXING
  //float Y_scale = *(float*)FE_YSCALE_ADDRESS + testvarX;
  //float Y_pos = ((resY - 600.0) / 2) + testvarY;
  float Y_scale = resY / 800.0;
  float Y_pos = resY / 8;
  
  // hax for 21:9 or 64:27
  if (aspect >= (21.0 / 9.0))
  {
	  Y_scale = 1;
	  Y_pos = resY / 4.5;
  }

 // float Y_scale = testvarX;
 // float Y_pos = testvarY;
 // printf("TESTVAR_X: %x | TESTVAR_Y = %x\n", &testvarX, &testvarY);

  v8 = (double)(*(int*)(a2 + 4) + *(int*)(a2 + 16)) * Y_scale + Y_pos;
  v10 = (double)*(int *)(a2 + 4) * Y_scale + Y_pos;

  v9 = (double)(*(int*)(a2 + 8) + *(int*)(a2 + 12)) * *(float*)FE_XSCALE_ADDRESS + *(float*)FE_XPOS_ADDRESS;
  v11 = (double)*(int *)(a2 + 8) * *(float*)FE_XSCALE_ADDRESS + *(float*)FE_XPOS_ADDRESS;
  
  
  *(int*)(a1 + 8) = (int)v11;
  *(int*)(a1 + 12) = (int)(v9 - v11);
  *(int*)a1 = 0x65E010;
  *(int*)(a1 + 4) = (int)v10;
  *(int*)(a1 + 16) = (int)(v8 - v10);
  return a1;
}

int __stdcall sub_463090_hook(int unk1, int X, int Y, int unk4, int unk5) // text rendering positions
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	//printf("sub_463090( X: %d Y: %d )\n", X, Y);
	//printf("TESTVAR_X: %x | TESTVAR_Y = %x\n", &testvarX, &testvarY);
	//X -= testvarX;
	//X = (int)((float)X * (resX_43f / 800.0)); // this scales almost perfectly
	//Y = (int)((float)Y * ((float)resY / 600.0));
	return sub_463090(thethis, unk1, X, Y, unk4, unk5);
}

int __stdcall sub_59B840_hook(char *key, mBorders* unk1, unsigned int unk2)
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	unsigned int retval = sub_59B840(thethis, key, (int)unk1, unk2);

	// TEMPORARY HUD FIX
	//*(int*)(unk1 + 0x4) = (int)FE_verposition; // Y POS
	//*(int*)(unk1 + 0x8) += (int)FE_horposition; // X POS
	//*(float*)FE_YSCALE_ADDRESS = resY / 600.0;

	// TEST!
	if (FixHUD > 1)
		(*unk1).botX = resX_600height;

	if (FixHUD == 1)
		*(float*)FE_XPOS_ADDRESS = FE_horposition;

	//*(int*)(unk1+0xC) = resX;
	//*(int*)(unk1+0x10) = resY;

	return retval;
}

int __stdcall sub_59B6E0_hook(char *key, unsigned int unk1, unsigned int unk2) // hook for fixing specific UI elements' mBorders
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	unsigned int retval = sub_59B6E0(thethis, key, unk1, unk2);
	CurrentFEChild = *(char**)(unk1 + 4);
	//if (retval)
	//	printf("Current FE child: %s\n", CurrentFEChild);

	return retval;
}

int __stdcall sub_59B840_hook_2(char *key, mBorders* unk1, unsigned int unk2) // hook for fixing specific UI elements' mBorders
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	CurrentFEElement = *(char**)(thethis + 8);
	unsigned int retval = sub_59B840(thethis, key, (int)unk1, unk2);

	// TEST!
	if (FixHUD > 1)
	{
		(*unk1).topX *= xscale_800;
		if (((*unk1).botX == 800) || ((*unk1).botX == 640))
		{
			(*unk1).botX = resX_600height;
		}
		//printf("TESTVAR_X = %X\nTESTVAR_Y = %X\n", &testvarX, &testvarY);



	//printf("TESTVAR_X = %X\n", &testvarX);

	//if (strcmp(CurrentFEElement, "<TEXT>.FlashText") == 0)
	//{
	//	printf("Rendering map! %s | ADDRESS mBorders: %X\n", CurrentFEElement, unk1);
	//	printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
	//	//(*unk1).botX = 0;
	//}

	// fix for hud - this will have to be coded specially for every element that is broken...
	//if (strcmp(CurrentFEElement, "s1.HudText") == 0)
	//{
	//	printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
	//	printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
	//	//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.63;
	//	//(*unk1).topX = resX_600height * (1 - (*unk1).topX / resX_600height);
	//	//printf("Rendering stats label 1! %s | ADDRESS mBorders: %X\n", CurrentFEElement, unk1);
	//	//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
	//}
		if (strcmp(CurrentFEElement, "s1.HudText") == 0)
		{
			(*unk1).topX = resX_600height - 82;
			(*unk1).topY = 2;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "s2.HudText") == 0)
		{
			(*unk1).topX = resX_600height - 82;
			(*unk1).topY = 19;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.63;
			//(*unk1).topX = resX_600height * (1 - (*unk1).topX / resX_600height);
			//printf("Rendering stats label 1! %s | ADDRESS mBorders: %X\n", CurrentFEElement, unk1);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "s3.HudText") == 0)
		{
			(*unk1).topX = resX_600height - 82;
			(*unk1).topY = 36;
		}

		if (strcmp(CurrentFEElement, "sl1.HudText") == 0)
		{
			(*unk1).topX = resX_600height - (*unk1).botX - 98;
			(*unk1).topY = 2;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.63;
			//(*unk1).topX = resX_600height * (1 - (*unk1).topX / resX_600height);
			//printf("Rendering stats label 1! %s | ADDRESS mBorders: %X\n", CurrentFEElement, unk1);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "sl2.HudText") == 0)
		{
			(*unk1).topX = resX_600height - (*unk1).botX - 96;
			(*unk1).topY = 19;
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.63;
			//(*unk1).topX = resX_600height * 0.6;
			//(*unk1).topX = (*unk1).topX * 0.1;
			//printf("Rendering stats label 2! %s | ADDRESS mBorders: %X\n", CurrentFEElement, unk1);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "sl3.HudText") == 0)
		{
			(*unk1).topX = resX_600height - (*unk1).botX - 97;
			(*unk1).topY = 36;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.63;
			//(*unk1).topX = resX_600height * 0.6;
			//(*unk1).topX = (*unk1).topX * 0.1;
			//printf("Rendering stats label 3! %s | ADDRESS mBorders: %X\n", CurrentFEElement, unk1);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "p1.HudText") == 0)
		{
			(*unk1).topX = 16;
			(*unk1).topY = 0;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "p2.HudText") == 0)
		{
			(*unk1).topX = 64;
			(*unk1).topY = 14;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "score.HudText") == 0)
		{
			(*unk1).topX = 0;
			(*unk1).topY = 64;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "map.HudMap") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 473;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<TEXT>.FlashText") == 0)
		{
			if ((*unk1).topX == 0 && (*unk1).topY == 125 && (*unk1).botX == 142 && (*unk1).botY == 12)
			{
				(*unk1).topX = 0;
				(*unk1).topY = -16;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bi1.HudStaticArt") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 2;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bi2.HudStaticArt") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 21;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bi3.HudStaticArt") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 40;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bt1.HudText") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 40;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bt2.HudText") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 21;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "bt3.HudText") == 0)
		{
			(*unk1).topX = 2;
			(*unk1).topY = 2;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}


		// RVM start
		if (strcmp(CurrentFEElement, "rearview.HudRearView") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//(*unk1).topX = 2;
			(*unk1).topY = 4;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEShape, "[PC:H_MIRROR.TPG.PCD] edge") == 0)
		{
			if (strcmp(CurrentFEElement, "i0.GStaticImage") == 0)
			{
				(*unk1).topX = -8;
				(*unk1).topY = -3;
			}
			if (strcmp(CurrentFEElement, "i0a.GStaticImage") == 0)
			{
				(*unk1).topX = 199;
				(*unk1).topY = -3;
			}
		}
		// RVM end

		// tach start

		if (strcmp(CurrentFEElement, "digitach.HudTachometer") == 0)
		{
			(*unk1).topX = resX_600height - 60;
			(*unk1).topY += 20; // this will need fixing if we're gonna do 1:1 scaling...
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEShape, "[PC:H_TACH.TPG.PCD] bck1") == 0)
		{
			(*unk1).topX = -72;
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2);
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//printf("Rendering tach! %s | ADDRESS mBorders: %X\n", CurrentFEElement, unk1);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//(*unk1).botX = 0;
		}
		if (strcmp(CurrentFEElement, "5500.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / -(32.958333333333333333333333333333);
			(*unk1).topX = -24;
			(*unk1).topY = 36;
		}
		if (strcmp(CurrentFEElement, "6000.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / -(46.529411764705882352941176470588);
			(*unk1).topX = -17;
			(*unk1).topY = 34;
		}
		if (strcmp(CurrentFEElement, "6500.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / -(79.1);
			//(*unk1).topX = -7;
			//(*unk1).topY = 31;
			(*unk1).topX = -10;
			(*unk1).topY = 32;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "7000.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / -(396);
			//(*unk1).topX = 0;
			//(*unk1).topY = 31;
			(*unk1).topX = -2;
			(*unk1).topY = 32;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "7500.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 132;
			//(*unk1).topX = 8;
			//(*unk1).topY = 31;
			(*unk1).topX = 6;
			(*unk1).topY = 32;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "8000.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 65.916666666666666666666666666667;
			//(*unk1).topX = 14;
			//(*unk1).topY = 31;
			(*unk1).topX = 12;
			(*unk1).topY = 32;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "8500.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 41.631578947368421052631578947368;
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 0.04545454545454545454545454545455;
			//(*unk1).topX = 20;
			//(*unk1).topY = 32;
			(*unk1).topX = 19;
			(*unk1).topY = 34;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "9000.GSimpleImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 30.307692307692307692307692307692;
			//(*unk1).topX = (resX_600height - (*unk1).botX) / testvarX;
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 0.05725190839694656488549618320611;
			//(*unk1).topX = 27;
			//(*unk1).topY = 34;
			(*unk1).topX = 26;
			(*unk1).topY = 36;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "speed.HudText") == 0)
		{
			if ((*unk1).botX == 84 && (*unk1).botY == 17)
				(*unk1).topX = -35;
		}
		if (strcmp(CurrentFEElement, "gear.HudText") == 0)
		{
			if ((*unk1).botX == 36 && (*unk1).botY == 16)
				(*unk1).topX = 4;
		}
		// tach end

		// dash start
		if (strcmp(CurrentFEElement, "dash.HudDashboard") == 0)
		{
			//(*unk1).topX = 20;
			//(*unk1).topY = 32;
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			(*unk1).topY -= 6;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "speed.HudText") == 0)
		{
			if (!((*unk1).botX == 84 && (*unk1).botY == 17))
				(*unk1).topX /= xscale_800;
			//(*unk1).topY = 2;
		}
		if (strcmp(CurrentFEElement, "gear.HudText") == 0)
		{
			if (!((*unk1).botX == 36 && (*unk1).botY == 16))
				(*unk1).topX /= xscale_800;
			//(*unk1).topY = 13;
		}
		if (strcmp(CurrentFEElement, "speedo.HudSpeedometer") == 0)
		{
			(*unk1).topX /= xscale_800;
			//(*unk1).topY = 32;
		}
		if (strcmp(CurrentFEElement, "tach.HudTachometer") == 0)
		{
			(*unk1).topX /= xscale_800;
			//(*unk1).topY = 56;
		}
		// dash end

		// EA Trax Chyron start
		if (strcmp(CurrentFEElement, "<ARTIST>.GText") == 0)
		{
			(*unk1).topX = 0;
		}
		if (strcmp(CurrentFEElement, "<TITLE>.GText") == 0)
		{
			(*unk1).topX = 0;
		}
		if (strcmp(CurrentFEElement, "<ALBUM>.GText") == 0)
		{
			(*unk1).topX = 0;
		}
		if (strcmp(CurrentFEElement, "<BRAND>.GText") == 0)
		{
			(*unk1).topX = 0;
		}
		if (strcmp(CurrentFEElement, "b1.GImageBox") == 0)
		{
			if ((*unk1).botX == 175 && (*unk1).botY == 17)
			{
				(*unk1).topX = -2;
			}
		}
		if (strcmp(CurrentFEElement, "b2.GImageBox") == 0)
		{
			if ((*unk1).botX == 175 && (*unk1).botY == 17)
			{
				(*unk1).topX = -2;
			}
		}
		if (strcmp(CurrentFEElement, "a1.GImageBox") == 0)
		{
			if ((*unk1).botX == 176 && (*unk1).botY == 71)
			{
				(*unk1).topX = -2;
			}
		}
		if (strcmp(CurrentFEElement, "a1b.GImageBox") == 0)
		{
			if ((*unk1).botX == 76 && (*unk1).botY == 71)
			{
				(*unk1).topX = -78;
			}
		}
		if (strcmp(CurrentFEElement, "b1b.GImageBox") == 0)
		{
			if ((*unk1).botX == 76 && (*unk1).botY == 17)
			{
				(*unk1).topX = -78;
			}
		}
		if (strcmp(CurrentFEElement, "b2b.GImageBox") == 0)
		{
			if ((*unk1).botX == 76 && (*unk1).botY == 17)
			{
				(*unk1).topX = -78;
			}
		}
		// EA Trax Chyron end

		// cuffometer
		if (strcmp(CurrentFEElement, "<CUFFOMETER>.WCuffometer") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			(*unk1).topY = (600 - (*unk1).botY) - 2;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<star0>.GStaticImage") == 0)
		{
			(*unk1).topX = 26;
			(*unk1).topY = 16;
		}
		if (strcmp(CurrentFEElement, "<star1>.GStaticImage") == 0)
		{
			(*unk1).topX = 46;
			(*unk1).topY = 16;
		}
		if (strcmp(CurrentFEElement, "<star2>.GStaticImage") == 0)
		{
			(*unk1).topX = 66;
			(*unk1).topY = 16;
		}
		if (strcmp(CurrentFEElement, "<star3>.GStaticImage") == 0)
		{
			(*unk1).topX = 87;
			(*unk1).topY = 16;
		}
		if (strcmp(CurrentFEElement, "<star4>.GStaticImage") == 0)
		{
			(*unk1).topX = 107;
			(*unk1).topY = 16;
		}

		if (strcmp(CurrentFEElement, "Reset.FlashText") == 0)
		{
			(*unk1).topX = -24;
			(*unk1).topY = -16;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<bar>.ProgressBar") == 0)
		{
			(*unk1).topX = 5;
			(*unk1).topY = 2;
		}

		if (strcmp(CurrentFEElement, "<bar2>.ProgressBar") == 0)
		{
			(*unk1).topX = 5;
			(*unk1).topY = 2;
		}

		if (strcmp(CurrentFEElement, "i1.GImageBox") == 0)
		{
			if ((*unk1).botX == 75 && (*unk1).botY == 14)
			{
				(*unk1).topX = 47;
				(*unk1).topY = 20;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		// cuffometer end

		// letterbox scaling
		if ((strcmp(CurrentFEElement, "boxT.GImageBox") == 0) || (strcmp(CurrentFEElement, "tttt.GImageBox") == 0 || (strcmp(CurrentFEElement, "T.GImageBox") == 0)))
		{
			(*unk1).botY = (600 - (resX_600height / LETTERBOX_ASPECT)) / 2;
			//printf("Rendering letterbox! %s | ADDRESS BotX: %X\n", CurrentFEElement, &(*unk1).botX);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//(*unk1).botX = 0;
		
		}
		
		if ((strcmp(CurrentFEElement, "boxB.GImageBox") == 0) || (strcmp(CurrentFEElement, "bbbb.GImageBox") == 0 || (strcmp(CurrentFEElement, "B.GImageBox") == 0)))
		{
			(*unk1).botY = (600 - (resX_600height / LETTERBOX_ASPECT)) / 2;
			(*unk1).topY += 110 - (*unk1).botY;
			//printf("Rendering letterbox! %s | ADDRESS BotX: %X\n", CurrentFEElement, &(*unk1).botX);
		//	printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//(*unk1).botX = 0;
		}

		// title screen & load screen
		// logo
		if (strcmp(CurrentFEElement, "LOGO.GStaticImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 1.85;
			//(*unk1).topX = resX_600height * 0.35;
			//(*unk1).topX = aspect * 159 * FOUR_BY_THREE_ASPECT;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			//(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.0889;
			(*unk1).botX = 0;
			(*unk1).botY = 0;
		}
		if (strcmp(CurrentFEElement, "logo.GStaticImage") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 1.85;
			//(*unk1).topX = resX_600height * 0.35;
			//(*unk1).topX = aspect * 159 * FOUR_BY_THREE_ASPECT;
			(*unk1).botX = 0;
			(*unk1).botY = 0;
		}
		if (strcmp(CurrentFEShape, "[PC:S_INTRO.LYR.PCD] nfsl") == 0)
		{
			//(*unk1).topX = pow(aspect, 2) * 90;
			//(*unk1).topX = 435;
			//(*unk1).topX *= 1 + (aspect - FOUR_BY_THREE_ASPECT); // this formula needs some work
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "EATip.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "loadbar.GStaticImage") == 0)
		{
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			(*unk1).topX = (resX_600height / 2) - 70;
		}
		
		if (strcmp(CurrentFEElement, "<BAR>.GImageBox") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 2.21;
			(*unk1).topX = (resX_600height / 2) - 64;
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
		}
		if (strcmp(CurrentFEElement, "bl.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
		}
		if (strcmp(CurrentFEElement, "cl.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
		}
		if (strcmp(CurrentFEElement, "ur.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
		}
		if (strcmp(CurrentFEElement, "br.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
		}
		if (strcmp(CurrentFEElement, "cr.GImageBox") == 0)
		{
			(*unk1).botX = resX_600height / 2;
		}
		// titlebar end

		// console fix
		if (strcmp(CurrentFEElement, "!console.GStaticImage") == 0)
		{
			(*unk1).botX = resX_600height * 0.77875;
		}
		// console end

		// button edges
		if (strcmp(CurrentFEElement, "L0.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
		}

		if (strcmp(CurrentFEElement, "R0.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
		}

		if (strcmp(CurrentFEElement, "L1.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
		}

		if (strcmp(CurrentFEElement, "R1.GImageBox") == 0)
		{
			(*unk1).topX /= xscale_800;
		}
		// FIX THESE TO CENTER OF THE SCREEN
		if (strcmp(CurrentFEElement, "!H0left.GImageBox") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 9;
			//(*unk1).topX /= xscale_800;
		}

		if (strcmp(CurrentFEElement, "!H0rite.GImageBox") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 1.6875;
			//(*unk1).topX /= xscale_800;
		}

		if (strcmp(CurrentFEElement, "!H1left.GImageBox") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 9;
			//(*unk1).topX /= xscale_800;
		}

		if (strcmp(CurrentFEElement, "!H1rite.GImageBox") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 1.6875;
			//(*unk1).topX /= xscale_800;
		}
		// button edges end

		// Dialogbox start
		if (strcmp(CurrentFEElement, "botart.GStaticImage") == 0)
		{
			(*unk1).topX = -4;
		}
		// Dialogbox end

		// Pause menu start
		if (strcmp(CurrentFEElement, "Accept.CmnPauseNav") == 0)
		{
			(*unk1).topX /= xscale_800;
		}
		// hud menu
		if (strcmp(CurrentFEElement, "<HUD>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 60)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "<SPEEDO>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 165)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "<MAP>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 95)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "<REAR_VIEW>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 130)
				(*unk1).topX /= xscale_800;
		}
		// gameopt menu
		if (strcmp(CurrentFEElement, "<JUMPCAM>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 60)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "<360CAM>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 233 && (*unk1).botY == 20 && (*unk1).topY == 95)
				(*unk1).topX /= xscale_800;
		}
		// audio menu
		//if (strcmp(CurrentFEElement, "RaceMusicVolume.CmnSlider") == 0)
		//{
		//	printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
		//	printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		//}
		//if (strcmp(CurrentFEElement, "frame.GFrame") == 0)
		//{
		//	printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
		//	printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		//}
		// Pause menu end

		// options menu
		//if (strcmp(CurrentFEElement, "<CAR_DETAIL>.CmnSlider") == 0)
		//{
		//	(*unk1).botX *= xscale_800;
		//}
		// options menu end

		// main menu stuff
		if (strcmp(CurrentFEElement, "i_back2.GSimpleImage") == 0)
		{
			//(*unk1).topX /= xscale_800;
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			//(*unk1).topX = resX_600height * 0.35;
			//(*unk1).topX = aspect * 159 * FOUR_BY_THREE_ASPECT;
			//(*unk1).botX = 0;
			//(*unk1).botY = 0;
			//printf("[%s][%s] GUI.mBorders: [ %d , %d ] %d , %d | ADDRESS: %X\n", CurrentFEShape, CurrentFEElement, (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY, unk1);
		}

		if (strcmp(CurrentFEElement, "i_logo.GSimpleImage") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<HotPursuit>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "<TopCop>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "<Championship>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "<SingleChallenge>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "<Multiplayer>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "<QuickRace>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "<Options>.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "p5.FEParticle") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2.5793650793650793650793650793651;
		}

		if (strcmp(CurrentFEElement, "p5a.FEParticle") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 1.8258426966292134831460674157303;
		}

		if (strcmp(CurrentFEElement, "p3.FEParticle") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 3.3695652173913043478260869565217;
		}

		if (strcmp(CurrentFEElement, "p3a.FEParticle") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 1.4168190127970749542961608775137;
		}

		if (strcmp(CurrentFEElement, "Back.GButton") == 0 || strcmp(CurrentFEElement, "back.GButton") == 0 || strcmp(CurrentFEElement, "<BACK>.GButton") == 0 || strcmp(CurrentFEElement, "!Back.GText") == 0 || strcmp(CurrentFEElement, "Back.CmnScreenNav") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "Next.GButton") == 0 || strcmp(CurrentFEElement, "next.GButton") == 0 || strcmp(CurrentFEElement, "<NEXT>.GButton") == 0 || strcmp(CurrentFEElement, "!Next.GText" ) == 0 || strcmp(CurrentFEElement, "Next.CmnScreenNav") == 0)
		{
			(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.47;
		}

		// main menu stuff end

		// general options menu start
		if (strcmp(CurrentFEElement, "<VIEW>.HP2OptionsView") == 0)
		{
			(*unk1).botX *= xscale_800;
			OptionsMenuXsize = (*unk1).botX;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		// general options menu end

		// challenge setup start
		if (strcmp(CurrentFEElement, "<VIEW>.HP2SingleChallengeView") == 0)
		{
			(*unk1).botX *= xscale_800;
		}
		// challenge setup end

		// controller options start
		if (strcmp(CurrentFEElement, "B1.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 1)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "B2.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 41)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "B3.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 81)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "B4.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == 121)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "C1.GImageBox") == 0)
		{
			if ((*unk1).botX == 268 && (*unk1).botY == 160 && (*unk1).topY == 1)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "C2.GImageBox") == 0)
		{
			if ((*unk1).botX == 226 && (*unk1).botY == 160 && (*unk1).topY == 1)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "A1.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 160 && (*unk1).topY == 1)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "D1.GImageBox") == 0)
		{
			if ((*unk1).botX == 471 && (*unk1).botY == 20 && (*unk1).topY == -19)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "H1.GText") == 0)
		{
			if ((*unk1).botX == 264 && (*unk1).botY == 12 && (*unk1).topY == -16)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "H2.GText") == 0)
		{
			if ((*unk1).botX == 208 && (*unk1).botY == 12 && (*unk1).topY == -16)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "!h0L.GImageBox") == 0)
		{
			if ((*unk1).botX == 240 && (*unk1).botY == 2 && (*unk1).topY == -20)
				(*unk1).botX *= xscale_800;
		}
		if (strcmp(CurrentFEElement, "!h0R.GImageBox") == 0)
		{
			if ((*unk1).botX == 240 && (*unk1).botY == 2 && (*unk1).topY == -20)
				(*unk1).botX *= xscale_800;
		}
		// controller options end

		// credits screen start
		if (strcmp(CurrentFEElement, "!H0left.GImageBox") == 0)
		{
			(*unk1).botX *= xscale_800;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "!H1left.GImageBox") == 0)
		{
			(*unk1).botX *= xscale_800;
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
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
			}
			if (strcmp(CurrentFEElement, "<DELETE>.GButton") == 0)
			{
				if ((*unk1).botX == 222 && (*unk1).botY == 16 && (*unk1).topY == 240)
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
			}
			if (strcmp(CurrentFEElement, "<NEW>.GButton") == 0)
			{
				if ((*unk1).botX == 222 && (*unk1).botY == 16 && (*unk1).topY == 270)
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
			}
			if (strcmp(CurrentFEElement, "<SAVE>.GButton") == 0)
			{
				if ((*unk1).botX == 222 && (*unk1).botY == 16 && (*unk1).topY == 300)
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;
				//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
				//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			}
			if (strcmp(CurrentFEElement, "<DPL>.ToggleButton") == 0)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 15;
			}
			if (strcmp(CurrentFEElement, "<DSTATS>.GListBox") == 0)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 137;

			}
			if (strcmp(CurrentFEElement, "<CURRP>.GText") == 0)
			{
				(*unk1).topX = (OptionsMenuXsize - 143) / 2 - 143;
				//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
				//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			}
		}
		// driver profile end

		// car select
		if (strcmp(CurrentFEElement, "<CAR>.CarWidget") == 0)
		{
			//(*unk1).botX = resX;
			//(*unk1).botY = resY;
			//(*unk1).topX = testvarX;
			////(*unk1).topY = testvarY;
			(*unk1).topX = resX / 21.33334;
			if ((resX / 1280.0) < 2.0) // not sure when it's negative exactly
				(*unk1).topX = -(*unk1).topX;
			
			////(*unk1).topX = -(*unk1).topX;
			////(*unk1).topY = -(resY * 0.0694444445);
			////(*unk1).topY = -(*unk1).topY;
			//printf("TOPX: %d\nTOPY: %d\n", (*unk1).topX, (*unk1).topY);
		}

		if (strcmp(CurrentFEElement, "<Showcase>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}

		if (strcmp(CurrentFEElement, "<CAR_IL>.HPImgList") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<TRANY>.ToggleButton") == 0)
		{
			if ((*unk1).botX == 182 && (*unk1).botY == 21 && (*unk1).topY == 241)
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2 + 78;
		}
		if (strcmp(CurrentFEElement, "<btl>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 45 && (*unk1).botY == 73 && (*unk1).topY == 42)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "<btr>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 45 && (*unk1).botY == 73 && (*unk1).topY == 42)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "p6.FEParticle") == 0)
		{
			if ((*unk1).botX == 128 && (*unk1).botY == 128 && (*unk1).topY == -6)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "p7.FEParticle") == 0)
		{
			if ((*unk1).botX == 128 && (*unk1).botY == 128 && (*unk1).topY == -6)
				(*unk1).topX /= xscale_800;
		}

		//if (strcmp(CurrentFEElement, "<btl>.RolloverButton") == 0)
		//{
		//	//(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		//	//(*unk1).topX = (resX_600height - (*unk1).botX) / -26.034482758620689655172413793103;
		//	//(*unk1).botX = 0;
		//	//(*unk1).botY = 0;
		//}

		// car select end


		// track select
		if (strcmp(CurrentFEElement, "<Descrip>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<tdir>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<lock>.GStaticImage") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<Cost>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		//if (strcmp(CurrentFEElement, "TRCK.ActorWidget") == 0)
		//{
		//	(*unk1).botX = 800;
		//}
		if (strcmp(CurrentFEElement, "<bup>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 36 && (*unk1).botY == 22 && (*unk1).topY == -3)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "<bdn>.RolloverButton") == 0)
		{
			if ((*unk1).botX == 35 && (*unk1).botY == 22 && (*unk1).topY == 68)
				(*unk1).topX /= xscale_800;
		}

		if (strcmp(CurrentFEElement, "p1.FEParticle") == 0)
		{
			if ((*unk1).botX == 25 && (*unk1).botY == 25 && (*unk1).topY == 33)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "p2.FEParticle") == 0)
		{
			if ((*unk1).botX == 25 && (*unk1).botY == 25 && (*unk1).topY == 33)
				(*unk1).topX /= xscale_800;
		}
		if (strcmp(CurrentFEElement, "<Dir>.GAnimImage") == 0)
		{
			if ((*unk1).botX == 45 && (*unk1).botY == 45 && (*unk1).topY == 22)
				(*unk1).topX /= xscale_800;
		}

		// track select end

		// event tree
		if (strcmp(CurrentFEElement, "<EVENT_OBJECTIVE>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<EVENT_STATUS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<EVENT_CAR>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<EVENT_OPPONENTS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<EVENT_LAPS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<EVENT_TRACKS>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<REWARD>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		//if (strcmp(CurrentFEElement, "<TREE>.TreeWidget") == 0)
		//{
		//	(*unk1).botX = 800;
		//	(*unk1).topX = 280;
		//}
		//if (strcmp(CurrentFEElement, "<TREE_DATA>.GText") == 0)
		//{
		//	(*unk1).botX = 800;
		//	(*unk1).topX = 280;
		//}
		// event tree end

		// starting/ending grid
		if (strcmp(CurrentFEElement, "t0.GText") == 0)
		{
			(*unk1).topX = 8;
			(*unk1).topY = 8;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "t0.FlashText") == 0)
		{
			(*unk1).topX = 8;
			(*unk1).topY = 8;
		}
		if (strcmp(CurrentFEElement, "GRID.GWidget") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<StartRace>.GButton") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "field.GWidget") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<LIST>.GListBox") == 0)
		{
			if ((*unk1).botX == 422 && (*unk1).botY == 160)
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<RESULTS>.GListBox") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "<TRACK>.GText") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "titlebar.GImageBox") == 0)
		{
			if ((*unk1).topX)
			{
				if ((*unk1).botX == 463 && (*unk1).botY == 40)
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				if ((*unk1).botX == 349 && (*unk1).botY == 30)
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2 - 57;
				if ((*unk1).botX == 464 && (*unk1).botY == 20)
					(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			}
		}
		if (strcmp(CurrentFEElement, "B1.GButton") == 0)
		{
			if ((*unk1).botX == 222 && (*unk1).botY == 18 && (*unk1).topY == 250)
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "B2.GButton") == 0)
		{
			if ((*unk1).botX == 222 && (*unk1).botY == 18 && (*unk1).topY == 290)
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "B3.GButton") == 0)
		{
			if ((*unk1).botX == 222 && (*unk1).botY == 18 && (*unk1).topY == 330)
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		if (strcmp(CurrentFEElement, "title.GText") == 0)
		{
			if ((*unk1).botX == 464 && (*unk1).botY == 30)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
				//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
				//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
			}
		}
		if (strcmp(CurrentFEElement, "h2.GText") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 2.9158878504672897196261682242991;
			if ((*unk1).botX == 176 && (*unk1).botY == 20)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) - 98;
			}
			if ((*unk1).botX == 164 && (*unk1).botY == 18)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) - 119;
			}
		}
		if (strcmp(CurrentFEElement, "h3.GText") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 1.4846153846153846153846153846154;
			if ((*unk1).botX == 221 && (*unk1).botY == 20)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) + 101;
			}
			if ((*unk1).botX == 166 && (*unk1).botY == 18)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) + 47;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "h4.GText") == 0)
		{
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 1.34765625;
			//if ((*unk1).botX == 166 && (*unk1).botY == 18)
			//{
			//	(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) + 47;
			//}
			if ((*unk1).botX == 110 && (*unk1).botY == 18)
			{
				(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) + 166;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}

		if (strcmp(CurrentFEElement, "<COLUMN0>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//(*unk1).topX = (resX_600height - (*unk1).botX) / 1.4846153846153846153846153846154;
			//if ((*unk1).botX == 26 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 1;
			//}

			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "<COLUMN1>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//if ((*unk1).botX == 177 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 26;
			//}
			//if ((*unk1).botX == 165 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 25;
			//}
		}
		if (strcmp(CurrentFEElement, "<COLUMN2>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//if ((*unk1).botX == 220 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 202;
			//}
			//if ((*unk1).botX == 167 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 189;
			//}
		}
		if (strcmp(CurrentFEElement, "<COLUMN3>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//if ((*unk1).botX == 220 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 202;
			//}
			//if ((*unk1).botX == 167 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 189;
			//}
		}
		if (strcmp(CurrentFEElement, "<COLUMN4>.GText") == 0)
		{
			(*unk1).topX /= xscale_800;
			//if ((*unk1).botX == 220 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 202;
			//}
			//if ((*unk1).botX == 167 && (*unk1).botY == 20)
			//{
			//	(*unk1).topX = 189;
			//}
		}

		if (strcmp(CurrentFEElement, "c2.GImageBox") == 0)
		{
			if ((*unk1).botX == 223 && (*unk1).botY == 160)
			{
				(*unk1).topX = 200;
			}
			if ((*unk1).botX == 167 && (*unk1).botY == 160)
			{
				(*unk1).topX = 188;
			}
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		if (strcmp(CurrentFEElement, "<TEXT>.GText") == 0)
		{
			if ((*unk1).botX == 464 && (*unk1).botY == 63 && (*unk1).topY == 253)
			{
				(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
			}
		}
		// starting/ending grid end

		// race end stuff start
		if (strcmp(CurrentFEElement, "02Points.GWidget") == 0)
		{
			(*unk1).topX = (resX_600height - (*unk1).botX) / 2;
		}
		// race end stuff end

		// misc. stuff
		if (strcmp(CurrentFEElement, "BNK1.GText") == 0)
		{
			(*unk1).topX = ((resX_600height - (*unk1).botX) / 2) * 1.63;
		}

		if (strcmp(CurrentFEElement, "tm.GText") == 0)
		{
			//(*unk1).topX = resX_600height  / 1.4622770919067215363511659807956;
			(*unk1).topX = ((resX_600height) / 2) + 196;
			//printf("GUI.mBorders pointers: [ %X , %X ] %X , %X\n", &(*unk1).topX, &(*unk1).topY, &(*unk1).botX, &(*unk1).botY);
			//printf("GUI.mBorders: [ %d , %d ] %d , %d\n", (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY);
		}
		// misc. stuff end

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

		//printf("[%s][%s] GUI.mBorders: [ %d , %d ] %d , %d | ADDRESS: %X\n", CurrentFEShape, CurrentFEElement, (*unk1).topX, (*unk1).topY, (*unk1).botX, (*unk1).botY, unk1);
		CurrentFEShape[0] = 0;

		//printf("sub_59B840: %X %X %X %X %X %X %X\n", *(int*)(thethis), *(int*)(thethis + 4), *(int*)(thethis + 8), *(int*)(thethis + 12), *(int*)(thethis + 16), *(int*)(thethis + 20), *(int*)(thethis + 24));
		
		//printf("%s\n", *(int*)(thethis + 8));
	}
	return retval;
}

int __cdecl sub_595440_null(int a1, int a2)
{
	return a1;
}

int __cdecl sub_595440(int a1, int a2) // scaler
{
	float v5; // [sp+14h] [bp+Ch]@1
	float v6; // [sp+14h] [bp+Ch]@1

	//printf("HOR: %X | VER: %X | ADDR1: %X | ADDR2: %X\n", *(signed int *)(a2 + 4), *(signed int *)(a2 + 8), a1, a2);

	//*(int *)(a2 + 8) = (int)((float)(*(int*)(a2 + 8) * 1.2));
	//*(int *)(a2 + 4) = (int)((float)(*(int*)(a2 + 4) * 1.2));

	v5 = (float)*(int*)(a2 + 8);
	v6 = (float)*(int*)(a2 + 4);
	//v6 += ((float)resX -  v6) / 2;


	*(int*)a1 = 0x65E1F8;
	*(int*)(a1 + 4) = (int)v6;
	*(int*)(a1 + 8) = (int)v5;
	return a1;
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

		//FE_horposition = 640.0f / (640.0f * (1.0f / resX * (resY / 480.0f)) * 2.0f);
		//FE_horposition = 240;
		FE_horscale = resX_43f / 800.0;
		FE_horposition = (resX - resX_43f) / 2;
		//FE_verposition = (resY - 600.0) / 2;

		*(float*)FE_XSCALE_ADDRESS = FE_horscale;
		if (FixHUD == 1)
			*(float*)FE_XPOS_ADDRESS = FE_horposition;

		// fix function scaling...
		*(float*)FE_FUNCTIONS_YSCALE_ADDRESS = 1 / (resY / 600.0);
		*(float*)FE_FUNCTIONS_XSCALE_ADDRESS = *(float*)FE_FUNCTIONS_YSCALE_ADDRESS;
		// hor function scaling breaks...
		//*(float*)FE_FUNCTIONS_XSCALE_ADDRESS = ((1 / aspect) * *(float*)FE_FUNCTIONS_YSCALE_ADDRESS) + *(float*)FE_FUNCTIONS_YSCALE_ADDRESS;

		//*(float*)FE_YSCALE_ADDRESS = resY / 600.0;
		//*(float*)FE_YPOS_ADDRESS = FE_verposition;
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
	xscale_800 = resX_600height / 800;

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

		//injector::MakeNOP(0x00547C46, 5, true);
		//injector::WriteMemory<int>(0x0084313C, 6, true);
	
		//injector::MakeNOP(0x004D67A7, 5, true);
		//EAGL_mallocinjector::MakeNOP(0x004D67B1, 5, true);
		//injector::MakeJMP(0x004D67A2, 0x004D67B6, true);
		//injector::WriteMemory<int>(0x006B8784, (int)&EAGL_Vertex_malloc, true);
		//injector::WriteMemory<int>(0x006B8788, (int)&EAGL_Vertex_free, true);
		//injector::WriteMemory<int>(0x55DDE8, (int)&EAGL_Vertex_malloc, true);
		//injector::WriteMemory<char>(0x55DDE4, 3, true);

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
	//	injector::WriteMemory<int>(0x0065ECF8, (int)&sub_463090_hook, true);
	//	//injector::WriteMemory<int>(0x463140, (int)&FE_horscale, true);
	//	//injector::WriteMemory<int>(0x5953D0, (int)&FE_horscale, true);
	//	//injector::WriteMemory<int>(0x595400, (int)&FE_horscale, true);
	//	//injector::WriteMemory<int>(0x005954E0, (int)&FE_fonthorscale, true);
	//	//injector::WriteMemory<int>(0x0046318B, (int)&FE_horscale, true); // stretches font graphics
	//	//injector::WriteMemory<int>(0x59546D, (int)&FE_horscale, true); // stretches graphics
	//
	//		injector::MakeCALL(0x445AB0 + 0x79, sub_595390_3Dhook, true); // affects FE 3D map actor rendering
	//		injector::MakeCALL(0x44D440 + 0x7B, sub_595390_3Dhook, true); // affects FE car actor rendering
	//		injector::MakeCALL(0x460660 + 0x42, sub_595390, true); // dynamic FE 2D objects?
	//		injector::MakeCALL(0x463090 + 0x96, sub_595390, true);
	//		injector::MakeCALL(0x463090 + 0x360, sub_595390, true);
	//		injector::MakeCALL(0x463610 + 0x4F, sub_595390, true); // vectorized FE graphics
	//		injector::MakeCALL(0x49C2C0 + 0x79, sub_595390_3Dhook, true); // affects FE 3D event tree actor rendering
	//	
	injector::MakeCALL(0x00462FDF, sub_5954A0, true);
	injector::MakeCALL(0x00463501, sub_5954A0, true);
	//	
	//		//
	//		injector::MakeCALL(0x463560 + 0x2F, sub_595440, true);
	//		injector::MakeCALL(0x463560 + 0x43, sub_595440, true);
	//	
	//		injector::MakeCALL(0x463780 + 0xA0, sub_595440, true);
	//		injector::MakeCALL(0x463780 + 0xC2, sub_595440, true);
	//		injector::MakeCALL(0x463780 + 0xE5, sub_595440, true);
	//		injector::MakeCALL(0x463780 + 0x108, sub_595440, true);
	//	
	//		injector::MakeCALL(0x463920 + 0x8B, sub_595440, true);
	//		injector::MakeCALL(0x463920 + 0xA1, sub_595440, true);
	//		injector::MakeCALL(0x463920 + 0xB5, sub_595440, true);
	//		injector::MakeCALL(0x463920 + 0xC9, sub_595440, true);
	//	
	//		injector::MakeCALL(0x463A50 + 0xA0, sub_595440, true);
	//		injector::MakeCALL(0x463A50 + 0xC2, sub_595440, true);
	//		injector::MakeCALL(0x463A50 + 0xE5, sub_595440, true);
	//		injector::MakeCALL(0x463A50 + 0x108, sub_595440, true);
	//	
	//		//injector::WriteMemory<float>(0x0046A46B, testvarY, true);
	//		//injector::WriteMemory<float>(0x0046A470, testvarX, true);
	//		//
	//		//injector::WriteMemory<float>(0x0046A4F3, testvarY, true);
	//		//injector::WriteMemory<float>(0x0046A4F8, testvarX, true);
	//		//
	//		//injector::WriteMemory<float>(0x0046A584, testvarY, true);
	//		//injector::WriteMemory<float>(0x0046A589, testvarX, true);
	//		//
	//		//injector::WriteMemory<float>(0x0046A612, testvarY, true);
	//		//injector::WriteMemory<float>(0x0046A617, testvarX, true);
	//	
	//		//injector::WriteMemory<float>(0x00469DFB, testvarX, true);
	//		//injector::WriteMemory<float>(0x00469E00, testvarX, true);
	//		//injector::WriteMemory<float>(0x00469E89, testvarX, true);
	//		//injector::WriteMemory<float>(0x00469E8E, testvarX, true);
	//		//injector::WriteMemory<float*>(0x46B4A8, &testvarY, true);
	//	
	//		injector::MakeCALL(0x4407B0 + 0x116, sub_46B470, true);
	//		injector::MakeCALL(0x4407B0 + 0x193, sub_46B470, true);
	//		injector::MakeCALL(0x4407B0 + 0x210, sub_46B470, true);
	//		injector::MakeCALL(0x4407B0 + 0x28A, sub_46B470, true);
	//		
	//		injector::MakeCALL(0x440A90 + 0xAF, sub_46B470, true);
	//		injector::MakeCALL(0x440A90 + 0x12B, sub_46B470, true);
	//		injector::MakeCALL(0x440A90 + 0x1A7, sub_46B470, true);
	//		injector::MakeCALL(0x440A90 + 0x226, sub_46B470, true);
	//		
	//		injector::MakeCALL(0x465D90 + 0x9E, sub_46B470, true);
	//		injector::MakeCALL(0x465D90 + 0x10A, sub_46B470, true);
	//		injector::MakeCALL(0x465D90 + 0x176, sub_46B470, true);
	//		injector::MakeCALL(0x465D90 + 0x1CE, sub_46B470, true);
	//		
	//		injector::MakeCALL(0x466050 + 0x9B, sub_46B470, true);
	//		injector::MakeCALL(0x466050 + 0x101, sub_46B470, true);
	//		injector::MakeCALL(0x469890 + 0xF2, sub_46B470, true);
	//		injector::MakeCALL(0x469890 + 0x189, sub_46B470, true);
	//		injector::MakeCALL(0x469890 + 0x220, sub_46B470, true);
	//		injector::MakeCALL(0x469890 + 0x2AB, sub_46B470, true);
	//		
	//		injector::MakeCALL(0x469C70 + 0xC1, sub_46B470, true);
	//		injector::MakeCALL(0x469C70 + 0x152, sub_46B470, true);
	//		injector::MakeCALL(0x469C70 + 0x1E0, sub_46B470, true);
	//		injector::MakeCALL(0x469C70 + 0x26E, sub_46B470, true);
	//		
	//		injector::MakeCALL(0x46A2C0 + 0x200, sub_46B470, true);
	//		injector::MakeCALL(0x46A2C0 + 0x28E, sub_46B470, true);
	//		injector::MakeCALL(0x46A2C0 + 0x31C, sub_46B470, true);
	//		injector::MakeCALL(0x46A2C0 + 0x3AA, sub_46B470, true);
	//	
	//		injector::MakeCALL(0x4407B0 + 0xDE, sub_46B4E0, true);
	//		injector::MakeCALL(0x4407B0 + 0x15B, sub_46B4E0, true);
	//		injector::MakeCALL(0x4407B0 + 0x1D8, sub_46B4E0, true);
	//		injector::MakeCALL(0x4407B0 + 0x255, sub_46B4E0, true);
	//		injector::MakeCALL(0x440A90 + 0x77, sub_46B4E0, true);
	//		injector::MakeCALL(0x440A90 + 0xF6, sub_46B4E0, true);
	//		injector::MakeCALL(0x440A90 + 0x172, sub_46B4E0, true);
	//		injector::MakeCALL(0x440A90 + 0x1F1, sub_46B4E0, true);
	//		injector::MakeCALL(0x469890 + 0xAC, sub_46B4E0, true);
	//		injector::MakeCALL(0x469890 + 0x143, sub_46B4E0, true);
	//		injector::MakeCALL(0x469890 + 0x1DA, sub_46B4E0, true);
	//		injector::MakeCALL(0x469890 + 0x265, sub_46B4E0, true);
	//		injector::MakeCALL(0x469C70 + 0x7B, sub_46B4E0, true);
	//		injector::MakeCALL(0x469C70 + 0x10C, sub_46B4E0, true);
	//		injector::MakeCALL(0x469C70 + 0x19A, sub_46B4E0, true);
	//		injector::MakeCALL(0x469C70 + 0x228, sub_46B4E0, true);
	//		injector::MakeCALL(0x46A2C0 + 0x1BA, sub_46B4E0, true);
	//		injector::MakeCALL(0x46A2C0 + 0x248, sub_46B4E0, true);
	//		injector::MakeCALL(0x46A2C0 + 0x2D6, sub_46B4E0, true);
	//		injector::MakeCALL(0x46A2C0 + 0x364, sub_46B4E0, true);
	//	
	//		injector::MakeCALL(0x54B3B0 + 0x86, sub_54B0D0, true);
	//		injector::MakeCALL(0x54B3B0 + 0xC6, sub_54B0D0, true);
	//		injector::MakeCALL(0x54B3B0 + 0x116, sub_54B0D0, true);
	//		injector::MakeCALL(0x54B3B0 + 0x152, sub_54B0D0, true);
	//		injector::MakeCALL(0x54B3B0 + 0x1A2, sub_54B0D0, true);
	//		injector::MakeCALL(0x54B3B0 + 0x1DE, sub_54B0D0, true);
	//		injector::MakeCALL(0x54B3B0 + 0x22D, sub_54B0D0, true);
	//		injector::MakeCALL(0x54B3B0 + 0x269, sub_54B0D0, true);
	//	
	//		//injector::WriteMemory<int>(0x0065EE80, (int)&sub_469890_hook, true);
			//injector::WriteMemory<int>(0x0065ECFC, (int)&sub_463920_hook, true);

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

	// sscanf border hook
		//injector::MakeCALL(0x0059B88C, sscanf_hook, true);

	}
	injector::MakeJMP(0x00538140, printf, true);

	// testing
	// SimGUIInterface
	//injector::MakeNOP(0x0045AC94, 5, true);
	//injector::MakeNOP(0x0045ACCC, 5, true);
	//injector::MakeCALL(0x005A15A4, FE_CursorPos, true);
	//injector::WriteMemory<int>(0x00604A95, (int)&testvarX, true);
	//injector::WriteMemory<int>(0x00604AB7, (int)&testvarY, true);
	//injector::WriteMemory<int>(0x00604ADD, (int)&testvarY, true);
	//injector::MakeNOP(0x00537FD3, 5, true);

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

