#pragma once
#include <detours.h>
#include "libgmavi.h"

extern void UnprotectModule(const char* moduleName = NULL);

typedef struct frameData_s
{
	std::string		name;
	std::string		path;
	uint32_t		frameNum;
	uint32_t		streamNum;
	bool			writeToDisk;
	uint8_t*		image_buffer;
}	frameData_t;

template <typename T>
T Hook(uint32_t address, void(*hook))
{
	return (T)DetourFunction((PBYTE)address, (PBYTE)hook);
}


extern uint32_t FindPattern(std::string pattern, uint32_t dataOffset = 0, bool abortOnFailure = true, const char* moduleName = NULL);

extern void MakeCall(BYTE* address, uint32_t hook, std::size_t len);

extern uint32_t SL_ConvertToString_Address;
extern char* SL_ConvertToString(short num);

typedef void(__cdecl * R_TakeScreenshot_t)(int width, int height, char* buffer);
extern R_TakeScreenshot_t R_TakeScreenshot;
extern bool Screenshot(frameData_t frameData);
extern void Screenshot_cod4(std::string path);

extern uint32_t	filenum;

extern bool AVStream_Start(std::vector <std::string> queue);
extern bool AVStream_Finish();
extern bool AddFrameToAVStream(frameData_t frameData);

extern void	update_outdir_path(std::string demoName);
extern std::string Update_FilePath(std::string path);

extern bool File_Exists(std::string path);
extern int Dir_Exists(const char* const path);
extern std::string cstr_to_string(char* str, int size);

extern uint32_t Con_IsVisible_Address;
extern bool Con_IsVisible();

extern bool IsKeyPressed(int key);

extern bool VerifyGameVersion(std::string supportedVersion);

extern void ToggleGreenscreen();

extern char* va(const char *fmt, ...);