#include "stdafx.hpp"
#include "Console.hpp"
#include "Utility.hpp"
#include "IW3D.hpp"

namespace Console 
{

	void PrintStartupInfo()
	{
		Log(CONSOLE_COLOR::YELLOW, "\n =========== IW3MVM 1.5.1 =========== \n");
		Log("  made by luckyy");
		Log("  http://codmvm.com/");
		Log("  http://twitter.com/reallyluckyy");
		Log("  http://youtube.com/luckyymvm \n");
		Log(CONSOLE_COLOR::YELLOW, " ==================================== \n\n");

		Log(CONSOLE_COLOR::BLUE, " GETTING STARTED\n");
		Log("  To get started, load up a demo using the 'demo' command and press F1 when ingame to open the menu!\n");
		Log("  From there on you can use the displayed hotkeys to toggle the respective features/settings.\n");
		Log("  You also might want to type 'mvm_' into the console to view a list of available dvars (fog, killfeed, ...).\n\n");
		Log(CONSOLE_COLOR::YELLOW, " DOLLYCAM\n");
		Log("  Start by going into freecam [F2] and placing at least 4 nodes [K] at different points in time.\n");
		Log("  You can then start the dollycam [J] to play your campath.\n");
		Log("  Alternatively you can turn on the frozen dollycam mode [F4] to do frozen cinematics.\n");
		Log("  NOTE: If your campath is skipped when starting the dollycam, try setting\n  mvm_dolly_skip to 0. You'll then be able to manually navigate to the starting tick\n  of your campath after starting the dollycam.\n\n");
		Log(CONSOLE_COLOR::YELLOW, " FOG\n");
		Log("  There are various dvars available for fog customization.\n");
		Log("  To start using custom fog settings, set mvm_fog_custom to 1!\n");
		Log("  After that is enabled, you can set\n    mvm_fog_color (color of custom fog)\n    mvm_fog_start (start distance of custom fog)\n    mvm_fog_end (end distance of custom fog)\n\n");
		Log(CONSOLE_COLOR::YELLOW, " KILLFEED\n");
		Log("  You can have the killfeed cycle through RGB colors by setting\n  mvm_killfeed_custom to 1!\n");
		Log("  Once that is set, you can adjust the speed using mvm_killfeed_speed.\n\n");
		Log(CONSOLE_COLOR::YELLOW, " DEPTH\n");
		Log("  Displaying a dynamic depthmap is possible by using the mvm_depth command!\n");
		Log("  Alternatively you can also use the hotkey [F5] to toggle the depth display.\n\n");
		Log(CONSOLE_COLOR::YELLOW, " GREENSCREEN\n");
		Log("  You can seperate the viewmodel/gun and player model from\n  the rest of the world by using mvm_greenscreen\n  or by pressing [F3]!\n\n");
		Log(CONSOLE_COLOR::YELLOW, " CAMERA ROLL\n");
		Log("  Rotating the camera is also possible by using the [MOUSEWHEEL].\n\n");
		Log(CONSOLE_COLOR::YELLOW, " CAMPATH EXPORTING\n");
		Log("  Use \"mvm_cam_export <filename>\" to save a campath to a file and \n    \"mvm_cam_import <filename>\" to load a campath from a file! \n\n");

#ifndef IW3D_EXPORT
		Log(CONSOLE_COLOR::BLUE, " RECORDING & STREAMS\n");
		Log("  Record without external software!\n  Set 'mvm_avidemo_fps' to your desired recording framerate and then press R to start recording to an AVI file.\n");
		Log("  You can also record depth, greenscreen and normal footage all at once with streams!\n  Use the 'mvm_streams_fps' and 'mvm_streams_passes' commands to set recording framerate and passes, then press R.\n\n");
#else
		Log(CONSOLE_COLOR::RED, " IW3D\n");
		Log("  Use 'iw3d_fps <fps>' to set the framerate at which 3D data is going to be exported.\n  Once set, the game will start recording the data to the 'IW3D' subfolder!\n\n");
#endif

		Log(CONSOLE_COLOR::BLUE, " REWINDING\n");
		Log("  The left arrow key now actually does what its supposed to, thanks to Anomaly#2268.\n\n");
	}

	void Init()
	{
		if (!AllocConsole())
		{
			MessageBoxA(NULL, "Could not create console window!", "ERROR", 1);
		}

		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);

		PrintStartupInfo();
	}

	static void SetConsoleColor(CONSOLE_COLOR color)
	{
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

		WORD attributes = 0;

		switch (color)
		{
		case WHITE:
			attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;
		case RED:
			attributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case YELLOW:
			attributes = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case GREEN:
			attributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case BLUE:
			attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		default:
			attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;

		}

		SetConsoleTextAttribute(handle, attributes);
	}

	void Log(CONSOLE_COLOR color, char* message, ...)
	{
		SetConsoleColor(color);

		char buffer[512];
		va_list args;
		va_start(args, message);
		vsprintf_s(buffer, 512, message, args);

		printf("%s\n", buffer);

		va_end(args);

		SetConsoleColor(CONSOLE_COLOR::WHITE);
	}

	void Log(char* message, ...)
	{
		SetConsoleColor(CONSOLE_COLOR::WHITE);

		char buffer[512];
		va_list args;
		va_start(args, message);
		vsprintf_s(buffer, 512, message, args);

		printf("%s\n", buffer);

		va_end(args);

	}
}