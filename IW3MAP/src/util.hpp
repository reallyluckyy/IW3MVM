#pragma once

#include "stdafx.hpp"
#include "game.hpp"

typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];

extern char* va(const char *fmt, ...);
extern char* SL_ConvertToString(short num);
extern float* UnpackPackedUnitVec(PackedUnitVec vec);
extern float* UnpackPackedTexCoords(PackedTexCoords tex);
extern void Demo_TogglePause();

class ProgressBar
{
public:

	void Start(std::string newTask, int* newProgress, int newGoal);

	void Update();

	void End();

private:

	std::chrono::milliseconds start;
	std::string task;
	int* progress;
	int goal;

	std::chrono::milliseconds GetEpochTime();
};