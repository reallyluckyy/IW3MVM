#pragma once
#include "Game.hpp"

#define IW3D_VERSION 0x00000002

extern float lastFovValue;

extern void IW3D_Frame();

extern void Patch_IW3D();

extern uint64_t iw3dFrameCount;

extern void DrawIW3DUI();