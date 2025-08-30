#pragma once

struct CameraView 
{
	bool isPOV;
	vec3_t position;
	vec3_t rotation;
};

extern CameraView currentView;

extern void Patch_Freecam();

extern uint32_t R_SetViewParmsForScene_Address;
extern uint32_t AnglesToAxisCall1_Address;
extern uint32_t AnglesToAxisCall2_Address;
extern uint32_t FX_SetupCamera_Address;
extern uint32_t CG_CalcFov_Address;
extern uint32_t CG_DObjGetWorldTagMatrix_Address;

extern uint32_t AnglesToAxis_Address;
extern uint32_t viewanglesX_Address; 
extern uint32_t viewanglesY_Address;
