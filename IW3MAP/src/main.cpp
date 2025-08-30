#include "main.hpp"
#include <thread>

#define EXPORT_CODM

typedef void* (__cdecl * DB_FindXAssetHeader_t)(int type, const char* name);
DB_FindXAssetHeader_t DB_FindXAssetHeader = (DB_FindXAssetHeader_t)0x489570;

dvar_s* Dvar_FindVar(std::string dvar)
{
	typedef dvar_s* (__cdecl * Dvar_FindVar_t)();
	Dvar_FindVar_t Dvar_FindVar_Internal = (Dvar_FindVar_t)0x56B5D0;

	const char* name = dvar.c_str();

	__asm mov edi, name
	return Dvar_FindVar_Internal();
}

#ifdef EXPORT_CODM
void ExportCodm(std::string directory, GfxWorld& gfxWorld)
{	
	Demo_TogglePause();

	std::vector<codm_Object> objects = LoadMapObjects(gfxWorld);
	Merge(objects);

	std::vector<codm_XModel> xmodels = LoadXModels(gfxWorld);

	std::fstream f;
	f.open(directory + "\\" + gfxWorld.baseName + ".codm", std::ios::binary);
	if (f.good())
	{
		f.close();
		printf("ERROR: %s already exists in %s!\nPlease delete the map file if you intend on re-exporting.\n\nPress any key to exit...", std::string(gfxWorld.baseName).append(".codm").c_str(), directory.c_str());
		std::cin.get();
		return;
	}

	std::ofstream file;
	file.open(directory + "\\" + gfxWorld.baseName + ".codm", std::ios::binary);
	WriteCodm(file, objects, xmodels);
	file.close();

	Demo_TogglePause();

	std::cout << "successfully exported codm" << std::endl;

	MessageBoxA(NULL, va("Successfully exported %s to %s!", std::string(gfxWorld.baseName).append(".codm").c_str(), directory.c_str()), "IW3MAP", 1);
}
#else
void ExportObj(std::string directory, GfxWorld& gfxWorld)
{
	std::ofstream file;
	file.open(directory + "\\" + gfxWorld.baseName + ".obj");
	WriteObj(file, gfxWorld);
	file.close();

	std::cout << "successfully exported obj" << std::endl;

	std::ofstream file;
	file.open(directory + "\\" + gfxWorld.baseName + ".mmf");
	WriteMmf(file, gfxWorld);
	file.close();

	std::cout << "successfully exported mmf" << std::endl;

	std::ofstream file;
	file.open(directory + "\\" + gfxWorld.baseName + ".mmat");
	WriteMmat(file, gfxWorld);
	file.close();

	std::cout << "successfully exported mmat" << std::endl;
}
#endif

void HideConsole()
{
	HWND window = GetConsoleWindow();
	FreeConsole();
	PostMessageA(window, WM_CLOSE, 0, 0);
}

void Init()
{
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONIN$", "r", stdin);

	clientinfo_t* clientinfo = (clientinfo_t*)0x839270;
	cg_s* cg = (cg_s*)0x74E338;

	std::string mapname(Dvar_FindVar("mapname")->current.string);

	if (mapname == "")
	{
		printf("ERROR: No map found!\nPlease load a map before loading the map exporter.\n\nPress any key to exit...");
		std::cin.get();

		HideConsole();
		return;
	}

	GfxWorld& gfxWorld = *(GfxWorld*)DB_FindXAssetHeader(0x10, va("maps/mp/%s.d3dbsp", Dvar_FindVar("mapname")->current.string));

	std::string mapsFolder = "IW3D\\maps";
	CreateDirectory("IW3D", NULL);
	CreateDirectory("IW3D\\maps", NULL);

#ifdef EXPORT_CODM
	ExportCodm(mapsFolder, gfxWorld);
#else
	ExportObj(mapsFolder, gfxWorld);
#endif

	HideConsole();
	return;
}

HMODULE module;

void __stdcall UnloadModule()
{
	Sleep(100);

	FreeLibraryAndExitThread(module, 0);
}

void UnloadDLL()
{
	std::thread(UnloadModule);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		module = hinstDLL;

		Init();
		UnloadDLL();
	}

	return FALSE;
}