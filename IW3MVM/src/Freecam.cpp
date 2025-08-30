#include "stdafx.hpp"
#include "Math.hpp"
#include "Console.hpp"
#include "Freecam.hpp"
#include "Utility.hpp"
#include "Game.hpp"
#include "Dollycam.hpp"
#include "Drawing.hpp"
#include "Demo.hpp"
#include "IW3D.hpp"
#include "Streams.hpp"

uint32_t R_SetViewParmsForScene_Address;
uint32_t AnglesToAxisCall1_Address;
uint32_t AnglesToAxisCall2_Address;
uint32_t FX_SetupCamera_Address;
uint32_t CG_CalcFov_Address;
uint32_t CG_DObjGetWorldTagMatrix_Address;

CameraView currentView;

void R_SetViewParmsForScene()
{
	/* DOLLYCAM PLAYING STUFF */

	if (!currentView.isPOV)
	{
		Node node;

		if (currentDollycamState == DollycamState::Waiting && !mvm_dolly_frozen->current.enabled)
		{
			DWORD startTick = (DWORD)nodes.begin()->first;
			if (Demo_GetCurrentTick() < startTick && mvm_dolly_skip->current.enabled)
			{
				DWORD diff = (startTick - Demo_GetCurrentTick()) / 10;
				if (diff > 500)
					diff = 500;
				if (diff < 5)
					diff = 5;

				Demo_SkipForward(diff);

				if (!currentView.isPOV)
					cg_draw2D->current.enabled = false;
				else
					cg_draw2D->current.enabled = true;
			}

			if (!mvm_dolly_skip->current.enabled)
			{
				if (frameCount % 60 == 0)
				{
					printf("mvm_dolly_skip is set to 0. Waiting for first tick...\n");
				}
			}
		}

		if (currentDollycamState != DollycamState::Disabled)
		{
			if (!mvm_dolly_frozen->current.enabled)
			{
				if (Demo_GetCurrentTick() >= nodes.begin()->first && Demo_GetCurrentTick() < nodes.rbegin()->first)
				{
					if (currentDollycamState == DollycamState::Waiting)
					{
						if (!Demo_IsPaused())
							Demo_TogglePause();

						timescale->current.value = dollyTimescale;
						com_maxfps->current.value = dollyMaxFps;
					}

					cg_draw2D->current.enabled = false;
					demoDrawMenu = false;

					currentDollycamState = DollycamState::Playing;
					node = GetNode(Demo_GetCurrentTick());

					cg_fov->current.value = (float)node.fov;

					currentView.position[0] = (float)node.x;
					currentView.position[1] = (float)node.y;
					currentView.position[2] = (float)node.z;

					currentView.rotation[0] = (float)node.pitch;
					currentView.rotation[1] = (float)node.yaw;
					currentView.rotation[2] = (float)node.roll;
				}
				else if (Demo_GetCurrentTick() > nodes.rbegin()->first)
				{
					// campath ended
					Console::Log(GREEN, "Campath ended!");

					//	Campath should really finish before this happens, since the last frame gets recorded too (which is not within the campath)
					if (recordingStatus == RecordingStatus::Running)
						recordingStatus = RecordingStatus::Finish;
					currentDollycamState = DollycamState::Disabled;
					timescale->current.value = dollyTimescale;
					com_maxfps->current.integer = maxfps_cap;
					currentView.rotation[2] = 0;
					demoDrawMenu = true;
				}
			}
			else 
			{
				
				// frozen campath
				if (currentDollycamState == DollycamState::Waiting)
				{
					frozenDollyBaseFrame = frameCount;
					frozenDollyCurrentFrame = frozenDollyBaseFrame;
					currentDollycamState = DollycamState::Playing;

					com_maxfps->current.integer = 60;
				}

				if (currentDollycamState == DollycamState::Playing && frozenDollyCurrentFrame < (frozenDollyBaseFrame + (nodes.size() - 1) * 5000))
				{
					cg_draw2D->current.enabled = false;
					demoDrawMenu = false;

					node = GetNode((double)(frozenDollyCurrentFrame - frozenDollyBaseFrame));

					cg_fov->current.value = (float)node.fov;

					currentView.position[0] = (float)node.x;
					currentView.position[1] = (float)node.y;
					currentView.position[2] = (float)node.z;

					currentView.rotation[0] = (float)node.pitch;
					currentView.rotation[1] = (float)node.yaw;
					currentView.rotation[2] = (float)node.roll;

					// while recording, we're handling this in the Advance stage
					if (!is_recording_streams && !is_recording_avidemo)
						frozenDollyCurrentFrame += (long)(mvm_dolly_frozen_speed->current.value * timescale->current.value);
				}
				else if (currentDollycamState == DollycamState::Playing && frozenDollyCurrentFrame >= (frozenDollyBaseFrame + (nodes.size() - 1) * 5000))
				{
					Console::Log(GREEN, "Campath ended!");

					if (recordingStatus == RecordingStatus::Running)
						recordingStatus = RecordingStatus::Finish;
					currentDollycamState = DollycamState::Disabled;
					currentView.rotation[2] = 0;
					demoDrawMenu = true;
				}
				
			}
		}
	}
	

	/* HANDLE FREECAM INPUT */

	if (!currentView.isPOV)
	{

		Dvar_FindVar("cg_thirdperson")->current.enabled = true;

		if (!Con_IsVisible())
		{
			float forwardSpeed, strafeSpeed, upSpeed;

			if (IsKeyPressed(0xA0)) // SHIFT
			{
				forwardSpeed = mvm_cam_speed->current.value * 1 * 2.5f;
				strafeSpeed = mvm_cam_speed->current.value * 0.9f * 2.5f;
				upSpeed = mvm_cam_speed->current.value * 0.8f * 2.5f;
			}
			else 
			{
				forwardSpeed = mvm_cam_speed->current.value * 1;
				strafeSpeed = mvm_cam_speed->current.value * 0.9f;
				upSpeed = mvm_cam_speed->current.value * 0.8f;
			}

			if (IsKeyPressed(0x57)) // W
			{
				currentView.position[0] += *player_lookat[0] * forwardSpeed;
				currentView.position[1] += *player_lookat[1] * forwardSpeed;
				currentView.position[2] += *player_lookat[2] * forwardSpeed;
			}

			if (IsKeyPressed(0x53)) // S
			{
				currentView.position[0] -= *player_lookat[0] * forwardSpeed;
				currentView.position[1] -= *player_lookat[1] * forwardSpeed;
				currentView.position[2] -= *player_lookat[2] * forwardSpeed;
			}

			if (IsKeyPressed(0x41)) // A
			{
				currentView.position[0] += *player_right[0] * forwardSpeed;
				currentView.position[1] += *player_right[1] * forwardSpeed;
				currentView.position[2] += *player_right[2] * forwardSpeed;
			}

			if (IsKeyPressed(0x44)) // D
			{
				currentView.position[0] -= *player_right[0] * forwardSpeed;
				currentView.position[1] -= *player_right[1] * forwardSpeed;
				currentView.position[2] -= *player_right[2] * forwardSpeed;
			}

			if (IsKeyPressed(0x51)) // Q
			{
				currentView.position[0] -= *player_up[0] * forwardSpeed;
				currentView.position[1] -= *player_up[1] * forwardSpeed;
				currentView.position[2] -= *player_up[2] * forwardSpeed;
			}

			if (IsKeyPressed(0x45)) // E
			{
				currentView.position[0] += *player_up[0] * forwardSpeed;
				currentView.position[1] += *player_up[1] * forwardSpeed;
				currentView.position[2] += *player_up[2] * forwardSpeed;
			}
		}

		/* SET FOV */

		refdef->tanHalfFovX = std::tan(Deg2Rad(Dvar_FindVar("cg_fov")->current.value * Dvar_FindVar("cg_fovscale")->current.value) * 0.5f);
		refdef->tanHalfFovY = refdef->tanHalfFovX * ((float)refdef->windowSize[1] / (float)refdef->windowSize[0]);

		/* SET CURRENT POSITION */

		refdef->position[0] = currentView.position[0];
		refdef->position[1] = currentView.position[1];
		refdef->position[2] = currentView.position[2];

	}
	else 
	{
		currentView.position[0] = refdef->position[0];
		currentView.position[1] = refdef->position[1];
		currentView.position[2] = refdef->position[2];

		Dvar_FindVar("cg_thirdperson")->current.enabled = false;
	}
}

uint32_t R_SetViewParmsForScene_tramp;
void __declspec(naked) hkR_SetViewParmsForScene()
{
	__asm pushad

	R_SetViewParmsForScene();

	__asm popad
	__asm jmp R_SetViewParmsForScene_tramp
}

uint32_t FX_SetupCamera_tramp;
void __declspec(naked) hkFX_SetupCamera()
{
	__asm pushad

	if (!currentView.isPOV)
	{
		refdef->position[0] = currentView.position[0];
		refdef->position[1] = currentView.position[1];
		refdef->position[2] = currentView.position[2];
	}

	__asm popad
	__asm jmp FX_SetupCamera_tramp
}

uint32_t viewanglesX_Address;
uint32_t viewanglesY_Address;

uint32_t AnglesToAxis_Address;
void __declspec(naked) hkAnglesToAxis()
{
	static float* angles;

	__asm pushad
	__asm mov angles, esi

	if (!currentView.isPOV)
	{
		if (currentDollycamState != DollycamState::Playing)
		{
			angles[0] = *(float*)viewanglesX_Address;
			angles[1] = *(float*)viewanglesY_Address;
			angles[2] = currentView.rotation[2];

			currentView.rotation[0] = *(float*)viewanglesX_Address * -1.0f;
			currentView.rotation[1] = *(float*)viewanglesY_Address;
		}
		else
		{
			angles[0] = currentView.rotation[0] * -1.0f;
			angles[1] = currentView.rotation[1];
			angles[2] = currentView.rotation[2];
		}
	}
	else 
	{
		currentView.rotation[0] = angles[0];
		currentView.rotation[1] = angles[1];
		currentView.rotation[2] = angles[2];
	}

	__asm popad
	__asm jmp AnglesToAxis_Address
}

void(*CG_CalcFov_tramp)(float);
void hkCG_CalcFov(float fov)
{
	if (!currentView.isPOV)
		fov = Dvar_FindVar("cg_fov")->current.value * Dvar_FindVar("cg_fovscale")->current.value;

#ifdef IW3D_EXPORT
	lastFovValue = fov;
#endif

	CG_CalcFov_tramp(fov);
}

uint32_t CG_DObjGetWorldTagMatrix_tramp;
float viewanglebuffer[9];
void __declspec(naked) hkCG_DObjGetWorldTagMatrix()
{
	static float* tempEDI;

	__asm mov tempEDI, edi
	__asm pushad

	if (!currentView.isPOV)
		tempEDI = viewanglebuffer;

	__asm popad
	__asm mov edi, tempEDI
	__asm jmp CG_DObjGetWorldTagMatrix_tramp
}

void Patch_Freecam()
{
	// make sure we're always in first person view whenever we first view a demo
	currentView.isPOV = true;

	// re-write the camera position when needed
	R_SetViewParmsForScene_tramp = Hook<uint32_t>(R_SetViewParmsForScene_Address, hkR_SetViewParmsForScene);
	MakeCall((BYTE*)AnglesToAxisCall1_Address, (uint32_t)hkAnglesToAxis, 5);
	MakeCall((BYTE*)AnglesToAxisCall2_Address, (uint32_t)hkAnglesToAxis, 5);

	// update position of world-space effects (such as smoke) with our new position
	FX_SetupCamera_tramp = Hook<uint32_t>(FX_SetupCamera_Address, hkFX_SetupCamera);

	// override fov when not in first person view
	CG_CalcFov_tramp = Hook<void(*)(float)>(CG_CalcFov_Address, hkCG_CalcFov);

#ifndef IW3D_EXPORT
	CG_DObjGetWorldTagMatrix_tramp = Hook<uint32_t>(CG_DObjGetWorldTagMatrix_Address, hkCG_DObjGetWorldTagMatrix);
#endif
	
}