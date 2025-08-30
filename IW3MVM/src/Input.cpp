#include "stdafx.hpp"
#include "Input.hpp"
#include "Utility.hpp"
#include "Freecam.hpp"
#include "Game.hpp"
#include "Drawing.hpp"
#include "Dollycam.hpp"
#include "Console.hpp"
#include "Demo.hpp"
#include "Streams.hpp"
#include "IW3D.hpp"

uint32_t CL_KeyEvent_Address;

void(*CL_KeyEvent_tramp)(int, Key, int, int);
void hkCL_KeyEvent(int a1, Key key, int down, int a4)
{
	// do not handle any key-presses whenever we're not in a demo
	if (!Demo_IsPlaying())
	{
		CL_KeyEvent_tramp(a1, key, down, a4);
		return;
	}

	if (down)
	{
		if (!Con_IsVisible())
		{
			if (!currentView.isPOV)
			{
				/* FREECAM-ONLY BINDS */

				switch (key)
				{
					case (Key)0x6A:

						if (is_recording_avidemo || is_recording_streams)
							return;

						if (currentDollycamState == DollycamState::Disabled)
							StartDolly();
						else
							StopDolly();

						buttonColorInfo.dollyplay[1] = 0.7f;
						buttonColorInfo.dollyplay[2] = 0.0f;

						break;

					case (Key)0x6B:
						AddNode();

						buttonColorInfo.dollyadd[1] = 0.7f;
						buttonColorInfo.dollyadd[2] = 0.0f;
						break;

					case (Key)0x6C:
						ClearNodes();

						buttonColorInfo.dollydel[1] = 0.7f;
						buttonColorInfo.dollydel[2] = 0.0f;
						break;
						

					case Key::MWHEELDOWN:
						if (GetAsyncKeyState(0xA0))
							cg_fov->current.value += 1.0f;
						else
							currentView.rotation[2] -= 2;
						break;

					case Key::MWHEELUP:
						if (GetAsyncKeyState(0xA0))
							cg_fov->current.value -= 1.0f;
						else
							currentView.rotation[2] += 2;
						break;


					case Key::MOUSE1:
						if (mvm_dolly_lmb->current.enabled)
						{
							AddNode();
						}
						break;
						

				}
			}

			/* GENERAL BINDS */

			switch (key)
			{
				case Key::UPARROW:
					if (timescale->current.value < 8.0f)
					{
						timescale->current.value += timescale->current.value < 1.0f ? 0.1f : 1.0f;

						if (timescale->current.value > 8.0f)
							timescale->current.value = 8.0f;

						buttonColorInfo.faster[1] = 0.7f;
						buttonColorInfo.faster[2] = 0.0f;
					}
					break;

				case Key::DOWNARROW:
					if (timescale->current.value > 0.1f)
					{
						timescale->current.value -= timescale->current.value <= 1.0f ? 0.1f : 1.0f;

						if (timescale->current.value < 0.1f)
							timescale->current.value = 0.1f;

						buttonColorInfo.slower[1] = 0.7f;
						buttonColorInfo.slower[2] = 0.0f;
					}
					break;

				case Key::RIGHTARROW:

					buttonColorInfo.forward[1] = 0.7f;
					buttonColorInfo.forward[2] = 0.0f;

					Demo_SkipForward(500);
					break;

				case Key::LEFTARROW:

					buttonColorInfo.back[1] = 0.7f;
					buttonColorInfo.back[2] = 0.0f;
					
					Demo_SkipBack();
					break;

				case Key::F1:
					demoDrawMenu = !demoDrawMenu;
					return;

				case Key::F2:
#ifdef IW3D_EXPORT
					// dont allow switching between freecam and pov while
					// recording with IW3D; that'd break the file
					if (!iw3d_fps->current.integer)
						currentView.isPOV = !currentView.isPOV;
#else
					if (!is_recording_streams && !is_recording_avidemo)
						currentView.isPOV = !currentView.isPOV;
#endif
					return;

				case Key::F3:
					ToggleGreenscreen();
					
					buttonColorInfo.greenscreen[1] = 0.7f;
					buttonColorInfo.greenscreen[2] = 0.0f;
					return;

				case Key::F4:
					if (nodes.size() > 0)
					{
						Console::Log(RED, "ERROR: cannot switch modes while nodes are placed\n");
						return;
					}

					mvm_dolly_frozen->current.enabled = !mvm_dolly_frozen->current.enabled;
					return;

				case Key::F5:

					{
						dvar_s* r_showfloatzdebug = Dvar_FindVar("r_showfloatzdebug");
						r_showfloatzdebug->current.integer = !r_showfloatzdebug->current.integer;

						if (r_showfloatzdebug->current.enabled)
						{
							Dvar_FindVar("r_dof_enable")->current.enabled = true;
							Dvar_FindVar("r_dof_tweak")->current.enabled = true;
						}
						else
						{
							Dvar_FindVar("r_dof_enable")->current.enabled = false;
							Dvar_FindVar("r_dof_tweak")->current.enabled = false;
						}
					}

					buttonColorInfo.depth[1] = 0.7f;
					buttonColorInfo.depth[2] = 0.0f;
					return;

				case Key::SPACE:

					// Make sure people dont accidentally play the demo while playing their frozen campath
					if (currentDollycamState != DollycamState::Disabled && mvm_dolly_frozen->current.enabled)
						return;
					
#ifndef IW3D_EXPORT
					if (is_recording_streams || is_recording_avidemo)
						return;
#endif 


					Demo_TogglePause();
					
					buttonColorInfo.pause[1] = 0.7f;
					buttonColorInfo.pause[2] = 0.0f;
					return;

				case Key::CTRL:
					currentMenu = currentMenu == MenuMode::Primary ? MenuMode::Secondary : MenuMode::Primary;
					return;

				case (Key)0x72: // R

					if (recordingStatus == RecordingStatus::Running)
						recordingStatus = RecordingStatus::Finish;
					else if (recordingStatus == RecordingStatus::Disabled)
					{
						// We only want the player to be able to record when either in POV, in the "Press X to play the cinematic" screen for a normal cinematic
						// or has placed a frozen cinematic which we'll then play for them

						bool isPlayingNormalCinematic = currentDollycamState == DollycamState::Playing && !mvm_dolly_frozen->current.enabled && Demo_IsPaused();
						uint32_t requiredNodeCount = mvm_dolly_linear->current.enabled ? 2 : 4;
						bool canPlayFrozenCinematic = currentDollycamState == DollycamState::Disabled && mvm_dolly_frozen->current.enabled && nodes.size() >= requiredNodeCount;
						if (currentView.isPOV || isPlayingNormalCinematic || canPlayFrozenCinematic)
						{

							if (mvm_avidemo_fps->current.integer > 0)
								is_recording_avidemo = true;
							else if (mvm_streams_fps->current.integer > 0)
								is_recording_streams = true;
							else
								return;

							//	Start frozen dolly when pressing R and we just started a recording
							if (!currentView.isPOV && mvm_dolly_frozen->current.enabled && !nodes.empty())
							{
								if (currentDollycamState == DollycamState::Disabled)
									StartDolly();
							}
						}
					}
						
					return;
			}
		}
	}

	CL_KeyEvent_tramp(a1, key, down, a4);
}

void Patch_Input()
{
	CL_KeyEvent_tramp = Hook<void(*)(int, Key, int, int)>(CL_KeyEvent_Address, hkCL_KeyEvent);
}