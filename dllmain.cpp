// NFSHP2 widescreen fix
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

volatile int resX = 1280;
volatile int resY = 720;
volatile float resX_43f = 960.0;
volatile float aspect;
volatile float aspect_diff = 0.75;


char UserDir[255];
char RenderCapsIni[255];
//bool bRerouteSaveDir = false;
bool bEnableConsole = false;
bool bFixHUD = true;
unsigned int RenderMemorySize = 0x732000;

float FE_horscale = 1.0;
float FE_fonthorscale = 1.0;
float FE_fontverscale = 1.0;
float FE_verscale = 1.0;
float FE_horposition = 0.0;
float FE_verposition = 0.0;

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

void InjectRes();
int InitRenderCaps();

struct UnkClass1
{
	void* vtable;
	int x;
	int y;
};

struct UnkClass2
{
	void* vtable;
	float y1;
	float x1;
	float x2;
	float y2;
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

	printf("GUI.mBorders: [ %d , %d ] %d , %d\nTESTVAR_X: %x\n", *(int*)topX, *(int*)topY, *(int*)botX, *(int*)botY, &testvarX);


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

int __stdcall sub_59B840_hook(char *key, unsigned int unk1, unsigned int unk2)
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	unsigned int retval = sub_59B840(thethis, key, unk1, unk2);

	// TEMPORARY HUD FIX
	//*(int*)(unk1 + 0x4) = (int)FE_verposition; // Y POS
	//*(int*)(unk1 + 0x8) += (int)FE_horposition; // X POS
	//*(float*)FE_YSCALE_ADDRESS = resY / 600.0;
	*(float*)FE_XPOS_ADDRESS = FE_horposition;

	//*(int*)(unk1+0xC) = resX;
	//*(int*)(unk1+0x10) = resY;

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
	if (bFixHUD)
	{
		resX_43f = ((float)resY * (4.0f / 3.0f));
		//FE_horposition = 640.0f / (640.0f * (1.0f / resX * (resY / 480.0f)) * 2.0f);
		//FE_horposition = 240;
		FE_horscale = resX_43f / 800.0;
		FE_horposition = (resX - resX_43f) / 2;
		//FE_verposition = (resY - 600.0) / 2;

		*(float*)FE_XSCALE_ADDRESS = FE_horscale;
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

	// METHOD 2 - FIX Y FE SCALING BY CHANGING SCALING FACTOR
	injector::WriteMemory<float>(0x00445BE3, aspect_diff, true); // FE 3D map actor Y scale
	injector::WriteMemory<float>(0x0044D59D, aspect_diff, true); // FE car actor Y scale
	injector::WriteMemory<float>(0x0049C3F3, aspect_diff, true); // FE 3D event tree actor Y scale

	return 0;
}

int InitConfig()
{
	CIniReader inireader("");
	const char* InputDirString = inireader.ReadString("HP2WSFix", "SaveDir", "save");
	bEnableConsole = inireader.ReadInteger("HP2WSFix", "EnableConsole", 0);
	bFixHUD = inireader.ReadInteger("HP2WSFix", "FixHUD", 1);
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

	// GUI hax
	if (bFixHUD)
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
	if (bFixHUD)
	{
		injector::MakeCALL(0x0045A8DC, sub_59B840_hook, true);
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
			// METHOD 2 - FIX Y FE SCALING BY CHANGING SCALING FACTOR
			injector::WriteMemory<float>(0x00445BE3, aspect_diff, true); // FE 3D map actor Y scale
			injector::WriteMemory<float>(0x0044D59D, aspect_diff, true); // FE car actor Y scale
			injector::WriteMemory<float>(0x0049C3F3, aspect_diff, true); // FE 3D event tree actor Y scale
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

