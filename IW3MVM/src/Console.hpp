#pragma once

enum CONSOLE_COLOR
{
	WHITE, RED, YELLOW, GREEN, BLUE
};

namespace Console 
{

	extern void Init();
	extern void Log(CONSOLE_COLOR color, char* message, ...);
	extern void Log(char* message, ...);
};