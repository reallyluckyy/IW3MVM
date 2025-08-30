#include "stdafx.hpp"
#include "Utility.hpp"
#include "Console.hpp"
#include "Game.hpp"

#include "Freecam.hpp"
#include "Input.hpp"
#include "Drawing.hpp"
#include "Demo.hpp"
#include "Misc.hpp"
#include "IW3D.hpp"
#include "Fog.hpp"


const std::string IW3MVM_SUPPORTED_VERSION = "CoD4 MP 1.7 build 568 nightly Wed Jun 18 2008 04:48:38PM win-x86";

void Init()
{
	// Remove write-protection from memory for patching
	UnprotectModule();

	// Create a console window for debugging output
	Console::Init();

	// Run this first since we need Dvar_FindVar
	Game_Init();

	if (gameVariant == GameVariant::COD4X) 
	{
		Console::Log(YELLOW, "\n=== COD4X WARNING ===\n");
		Console::Log(WHITE, "Since you're running COD4X with IW3MVM, please be aware that some features might not work correctly.\n");
		Console::Log(WHITE, "Additionally, please be careful when using the two in combination as I can not guarantee that any potential COD4X anticheat might not lead to unforeseen consequences.\n");
	}

	// Check whether the user is running a supported version of COD4
	if (gameVariant == GameVariant::Stock && !VerifyGameVersion(IW3MVM_SUPPORTED_VERSION))
	{
		Console::Log(YELLOW, "\n=== WARNING: You're not running 1.7! ===\n");
		Console::Log(WHITE, "Current version: ");
		Console::Log(YELLOW, "    %s\n", Dvar_FindVar("version")->current.string);
		Console::Log(WHITE, "Recommended version:");
		Console::Log(GREEN, "    %s\n\n", IW3MVM_SUPPORTED_VERSION.c_str());
		
		Console::Log(WHITE, "Some IW3MVM features might be disabled due to not being compatible with your current game version.\nI recommend installing COD4 1.7 if you want to use the disabled features below.\nYou can download the 1.7 iw3mp.exe from this link kindly hosted by AZSRY: \n");
		Console::Log(BLUE, "https://azsry.com/data/iw3mp.zip\n");
		Console::Log(WHITE, "(You'll need to replace the iw3mp.exe in your game directory with the given iw3mp.exe)");
	}

	Patch_Freecam();
	Patch_Input();
	Patch_Demo();
	Patch_Drawing();
	Patch_Misc();
	Patch_Fog();

#ifdef IW3D_EXPORT
	if (VerifyGameVersion(IW3MVM_SUPPORTED_VERSION))
		Patch_IW3D();
	else
		Console::Log(RED, "\n\nIW3D is disabled.\nPlease use the above-mentioned COD4 1.7 game executable to properly use IW3D!\n\n");
#endif
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD     fdwReason,
	LPVOID    lpvReserved
)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		Init();

	return TRUE;
}