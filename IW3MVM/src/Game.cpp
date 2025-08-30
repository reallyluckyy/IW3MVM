#include "stdafx.hpp"
#include "Game.hpp"
#include "Console.hpp"
#include "Utility.hpp"
#include "Freecam.hpp"
#include "Drawing.hpp"
#include "Demo.hpp"
#include "Misc.hpp"
#include "IW3D.hpp"
#include "Input.hpp"
#include "File.hpp"

#include <algorithm>
#include <ctime>
#include "Dollycam.hpp"

GameVariant gameVariant = GameVariant::Stock;
std::string cod4xModuleName;

refdef_t* refdef;

IDirect3DDevice9*	iw3_d3d9_device;
addressValue_t		toggle_console;
uint32_t*			window_isActive;
float*		realFov_divisor;

/* worldViewMatrix */
float*		player_pos[3];
float*		player_lookat[3];
float*		player_right[3];
float*		player_up[3];

float*		player_roll;

dvar_s* timescale;
dvar_s* cg_draw2D;
dvar_s* cg_fov;
dvar_s* cg_fovscale;
dvar_s* com_maxfps;

dvar_s* mvm_fog_color;
dvar_s* mvm_fog_custom;
dvar_s* mvm_fog_start;
dvar_s* mvm_fog_end;

dvar_s* mvm_cam_speed;

dvar_s* mvm_dolly_linear;
dvar_s* mvm_dolly_frozen;
dvar_s* mvm_dolly_skip;
dvar_s* mvm_dolly_lmb;
dvar_s* mvm_dolly_frozen_speed;

dvar_s* mvm_killfeed_custom;
dvar_s* mvm_killfeed_speed;

dvar_s* mvm_forcelod;

bool is_recording_streams = false;
dvar_s* mvm_streams_fps;
dvar_s* mvm_streams_passes;
dvar_s* mvm_streams_aeExport;
dvar_s* mvm_streams_pixfmt;
dvar_s* mvm_streams_aeExport_sun;

bool is_recording_avidemo = false;
dvar_s* mvm_avidemo_fps;
dvar_s* mvm_export_format;
dvar_s* mvm_streams_interval;
dvar_s* mvm_streams_depthFormat;

//	Unsure how to set string outside of console commands
dvar_s* mvm_output_directory;
std::string standard_output_directory;

BYTE* DebugShaderPatch;
BYTE* FullBrightPatch;

dvar_s* iw3d_fps;

/* Command functions */

uint32_t Cbuf_AddText_Address;
void Cbuf_AddText(std::string command)
{
	command.append("\n");

	const char* commandString = command.c_str();

	__asm
	{
		mov eax, commandString
		mov ecx, 0
		call Cbuf_AddText_Address
	}
}

uint32_t* cmd_arg_id;
uint32_t* cmd_argc;
uint32_t** cmd_argv;

int Cmd_Argc()
{
	return cmd_argc[*cmd_arg_id];
}

char* Cmd_Argv(int arg)
{
	if ((unsigned)arg >= cmd_argc[*cmd_arg_id])
	{
		return "";
	}

	return (char*)(cmd_argv[*cmd_arg_id][arg]);
}

cmd_function_t** cmd_functions;
cmd_function_t* Cmd_AddCommand(const char* name, void(*function)(void))
{
	cmd_function_t* cmd;

	cmd = (cmd_function_t*)malloc(sizeof(struct cmd_function_t));
	cmd->name = name;
	cmd->function = function;
	cmd->next = *cmd_functions;
	*cmd_functions = cmd;

	return cmd;
}

uint32_t Cmd_ExecuteSingleCommand_Address;
void Cmd_ExecuteSingleCommand(std::string command)
{
	typedef void(*Cmd_ExecuteSingleCommand_t)(std::int32_t, std::int32_t, const char*);
	Cmd_ExecuteSingleCommand_t Cmd_ExecuteSingleCommand_Internal = (Cmd_ExecuteSingleCommand_t)Cmd_ExecuteSingleCommand_Address;

	Cmd_ExecuteSingleCommand_Internal(0, 0, command.c_str());
}


/* Dvar functions */

uint32_t Dvar_FindVar_Address;
uint32_t Dvar_FindVar_Version = 0;
dvar_s* Dvar_FindVar(std::string dvar)
{
	const char* name = dvar.c_str();

	if (Dvar_FindVar_Version == 0)
	{
		typedef dvar_s* (__cdecl * Dvar_FindVar_t)();
		Dvar_FindVar_t Dvar_FindVar_Internal = (Dvar_FindVar_t)Dvar_FindVar_Address;

		__asm mov edi, name
		return Dvar_FindVar_Internal();
	}
	else
	{
		typedef dvar_s* (__cdecl * Dvar_FindVar_t)(const char* name);
		Dvar_FindVar_t Dvar_FindVar_Internal = (Dvar_FindVar_t)Dvar_FindVar_Address;

		return Dvar_FindVar_Internal(name);
	}
}

dvar_s* Dvar_RegisterBool(std::string name, bool defaultValue)
{
	Cmd_ExecuteSingleCommand(va("dvar_bool %s %i\n", name.c_str(), defaultValue));
	return Dvar_FindVar(name.c_str());
}

dvar_s* Dvar_RegisterInt(std::string name, int defaultValue, int min, int max)
{
	Cmd_ExecuteSingleCommand(va("dvar_int %s %i %i %i\n", name.c_str(), defaultValue, min, max));
	return Dvar_FindVar(name.c_str());
}

dvar_s* Dvar_RegisterFloat(std::string name, float defaultValue, float min, float max)
{
	Cmd_ExecuteSingleCommand(va("dvar_float %s %g %g %g\n", name.c_str(), defaultValue, min, max));
	return Dvar_FindVar(name.c_str());
}

uint32_t Dvar_RegisterString_Address;
uint32_t Dvar_RegisterString_Version = 0;
dvar_s* Dvar_RegisterString(std::string name, const char* defaultValue) 
{
	if (Dvar_RegisterString_Version == 0)
	{
		typedef dvar_s* (__cdecl* Dvar_RegisterString_Internal_t)(const char* dvarName, int flags, const char* description);
		Dvar_RegisterString_Internal_t Dvar_RegisterString_Internal = (Dvar_RegisterString_Internal_t)Dvar_RegisterString_Address;

		__asm mov ecx, defaultValue
		return Dvar_RegisterString_Internal(name.c_str(), 0x4000, "External Dvar");
	}
	else 
	{
		typedef dvar_s* (__cdecl* Dvar_RegisterString_Internal_t)(int flags);
		Dvar_RegisterString_Internal_t Dvar_RegisterString_Internal = (Dvar_RegisterString_Internal_t)Dvar_RegisterString_Address;

		const char* dvarName = name.c_str();
		const char* dvarDescription = "External Dvar";

		__asm mov edx, defaultValue
		__asm mov ecx, dvarDescription
		__asm mov edi, dvarName
		return Dvar_RegisterString_Internal(0x4000);
	}
}

uint32_t Dvar_RegisterColor_Address;
uint32_t Dvar_RegisterColor_Version = 0;
dvar_s* Dvar_RegisterColor(const char* dvarName, float r, float g, float b, float a, unsigned __int16 flags, const char* description)
{
	if (Dvar_RegisterColor_Version == 0)
	{
		typedef dvar_s* (__cdecl * Dvar_RegisterColor_t)(const char* dvarName, float r, float g, float b, float a, unsigned __int16 flags, const char* description);
		Dvar_RegisterColor_t Dvar_RegisterColor = (Dvar_RegisterColor_t)Dvar_RegisterColor_Address;

		return Dvar_RegisterColor(dvarName, r, g, b, a, flags, description);
	}
	else
	{
		typedef dvar_s* (__cdecl * Dvar_RegisterColor_t)(float r, float g, float b, float a, unsigned __int16 flags, const char* description);
		Dvar_RegisterColor_t Dvar_RegisterColor = (Dvar_RegisterColor_t)Dvar_RegisterColor_Address;

		__asm mov edi, dvarName
		return Dvar_RegisterColor(r, g, b, a, flags, description);
	}

	return 0;
}

void(*Cmd_ExecuteSingleCommand_tramp)(int, int, const char*);
void hkCmd_ExecuteSingleCommand(int a1, int a2, const char* command)
{
	Cmd_ExecuteSingleCommand_tramp(a1, a2, command);
	
	std::string cmd(command);
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

	while (cmd[0] == '/' || cmd[0] == '\\')
		cmd = cmd.substr(1);

#ifdef IW3D_EXPORT
	if (cmd.find("iw3d_fps") == 0)
	{
		std::string filepath = va("IW3D\\%s.iw3d", Demo_GetNameEscaped().c_str());

		if (iw3d_fps->current.integer == 0)
		{
			if (File_Exists(filepath) && iw3dFrameCount)
			{
				Dvar_FindVar("cl_avidemo")->current.integer = 0;
					
				// jump to beginning and write frame count
				FILE* f = fopen(filepath.c_str(), "r+b");

				fwrite(&iw3dFrameCount, 8, 1, f);

				fclose(f);

				File file(va("IW3D\\%s.iw3d", Demo_GetNameEscaped().c_str()), std::ios_base::app | std::ios::binary);
				file.WriteStringTable();
				file.Close();

				iw3dFrameCount = 0;
				printf("finished writing iw3d file (%llu frames)\n", iw3dFrameCount);
			}
		}
		else 
		{
			if (File_Exists(filepath))
			{
				Console::Log(RED, "\n%s already exists!\nPlease delete it if you intend on re-recording this demo.", filepath.c_str());
				iw3d_fps->current.integer = 0;
				return;
			}

			Dvar_FindVar("cl_avidemo")->current.integer = iw3d_fps->current.integer;

			File file(filepath, std::ios::out | std::ios::binary);

			file.Write(0xFFFFFFFFFFFFFFFF);
			file.Write(IW3D_VERSION);
			file.Write(std::time(nullptr));
			file.Write((uint8_t)(12)); // entity count

			file.Close();

			printf("starting iw3d recording..\n");
		}
	}
#else
	if (!is_recording_avidemo && !is_recording_streams)
	{
		// Make sure is EITHER streams or avidemo that is set to a value
		// since we use that to determine which one to start recording
		if (cmd.find("mvm_streams_fps") == 0)
		{
			mvm_avidemo_fps->current.integer = 0;
			mvm_avidemo_fps->latched.integer = 0;
		}
		else if (cmd.find("mvm_avidemo_fps") == 0)
		{
			mvm_streams_fps->current.integer = 0;
			mvm_streams_fps->latched.integer = 0;
		}
	}
#endif
}

void CameraImport();
void CameraExport();

void GenerateDefaultConfigs()
{
	const std::string mvm_configs[12] = {
		"mvm_w.cfg",
		"mvm_wd.cfg",
		"mvm_wn.cfg",
		"mvm_w-g.cfg",
		"mvm_w-gd.cfg",
		"mvm_w-gn.cfg",
		"mvm_gn.cfg",
		"mvm_gG.cfg",
		"mvm_gB.cfg",
		"mvm_gR.cfg",
		"mvm_gA.cfg",
		"mvm_hud.cfg"
	};

	const std::string mvm_configs_contents[12] = {
		"	//	IW3MVM DEFAULT CONFIG	-	WORLD PASS\n\
r_clearcolor 0 0 0 1\n\
r_clearcolor2 0 0 0 1\n\
r_clear 3\n\
r_skippvs 0\n\
fx_enable 1\n\
fx_marks 1\n\
r_zfar 0\n\
r_znear 4.0\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	WORLD-DEPTH PASS\n\
r_clearcolor 0 0 0 1\n\
r_clearcolor2 0 0 0 1\n\
r_clear 3\n\
r_skippvs 0\n\
fx_enable 1\n\
fx_marks 1\n\
r_zfar 0\n\
r_znear 4.0\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 1\n\
r_dof_tweak 1\n",
		"	//	IW3MVM DEFAULT CONFIG	-	WORLD-NORMALS PASS\n\
r_clearcolor 0 0 0 1\n\
r_clearcolor2 0 0 0 1\n\
r_clear 3\n\
r_skippvs 0\n\
fx_enable 1\n\
fx_marks 1\n\
r_zfar 0\n\
r_znear 0.2\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 1\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	WORLD-NOGUN PASS\n\
r_clearcolor 0 0 0 1\n\
r_clearcolor2 0 0 0 1\n\
r_clear 3\n\
r_skippvs 0\n\
fx_enable 1\n\
fx_marks 1\n\
r_zfar 0\n\
r_znear 4.0\n\
cg_drawgun 0\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	WORLD-NOGUN-DEPTH PASS\n\
r_clearcolor 0 0 0 1\n\
r_clearcolor2 0 0 0 1\n\
r_clear 0\n\
r_skippvs 0\n\
fx_enable 1\n\
fx_marks 1\n\
r_zfar 0\n\
r_znear 4.0\n\
cg_drawgun 0\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 1\n\
r_dof_tweak 1\n",
		"	//	IW3MVM DEFAULT CONFIG	-	WORLD-NOGUN-NORMALS PASS\n\
r_clearcolor 0 0 0 1\n\
r_clearcolor2 0 0 0 1\n\
r_clear 0\n\
r_skippvs 0\n\
fx_enable 1\n\
fx_marks 1\n\
r_zfar 0\n\
r_znear 0.2\n\
cg_drawgun 0\n\
cg_draw2d 0\n\
r_debugshader 1\n\
r_showfloatzdebug 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	GUN-NORMALS PASS\n\
r_clearcolor 0 0 0 1\n\
r_clearcolor2 0 0 0 1\n\
r_clear 0\n\
r_skippvs 1\n\
fx_enable 0\n\
fx_marks 0\n\
r_zfar 0.001\n\
r_znear 10000\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 1\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	GUN-GREEN PASS\n\
r_clearcolor 0 1 0 0\n\
r_clearcolor2 0 1 0 0\n\
r_clear 2\n\
r_skippvs 1\n\
fx_enable 0\n\
fx_marks 0\n\
r_zfar 0.001\n\
r_znear 10000\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	GUN-BLUE PASS\n\
r_clearcolor 0 0 1 0\n\
r_clearcolor2 0 0 1 0\n\
r_clear 2\n\
r_skippvs 1\n\
fx_enable 0\n\
fx_marks 0\n\
r_zfar 0.001\n\
r_znear 10000\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	GUN-RED PASS\n\
r_clearcolor 1 0 0 0\n\
r_clearcolor2 1 0 0 0\n\
r_clear 2\n\
r_skippvs 1\n\
fx_enable 0\n\
fx_marks 0\n\
r_zfar 0.001\n\
r_znear 10000\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	GUN-MATTE PASS\n\
r_clearcolor 1 1 1 0\n\
r_clearcolor2 1 1 1 0\n\
r_clear 2\n\
r_skippvs 1\n\
fx_enable 0\n\
fx_marks 0\n\
r_zfar 0.001\n\
r_znear 10000\n\
cg_drawgun 1\n\
cg_draw2d 0\n\
r_debugshader 0\n\
r_showfloatzdebug 1\n\
r_dof_tweak 0\n",
		"	//	IW3MVM DEFAULT CONFIG	-	HUD PASS\n\
r_clearcolor 0 1 0 0\n\
r_clearcolor2 0 1 0 0\n\
r_clear 2\n\
r_skippvs 1\n\
fx_enable 0\n\
fx_marks 0\n\
r_zfar 0.001\n\
r_znear 10000\n\
cg_drawgun 0\n\
cg_draw2d 1\n\
con_gamemsgwindow0msgtime 5\n\
cg_centertime 4\n\
hud_enable 0\n\
ui_hud_hardcore 1\n\
cg_drawcrosshair 0\n\
cg_drawcrosshairnames 0\n\
cg_drawfriendlynames 0\n\
cg_hudDamageIconHeight 0\n\
cg_hudDamageIconInScope 0\n\
cg_hudDamageIconOffset 0\n\
cg_hudDamageIconTime 0\n\
cg_hudDamageIconWidth 0\n\
hud_health_pulserate_critical .1\n\
hud_health_pulserate_injured .1\n\
hud_health_startpulse_critical 0\n\
hud_health_startpulse_injured 0\n\
hud_healthOverlay_phaseEnd_pulseDuration 0\n\
hud_healthOverlay_phaseEnd_toAlpha 0\n\
hud_healthOverlay_phaseOne_pulseDuration 0\n\
hud_healthOverlay_phaseThree_pulseDuration 0\n\
hud_healthOverlay_phaseThree_toAlphaMultiplier 0\n\
hud_healthOverlay_phaseTwo_pulseDuration 0\n\
hud_healthOverlay_phaseTwo_toAlphaMultiplier 0\n\
hud_healthOverlay_pulseStart 0\n\
hud_healthOverlay_regenPauseTime 0\n\
cg_overheadnamesize 0\n\
cg_overheadnamessize 0\n\
cg_overheadranksize 0\n\
g_hardcore 1\n\
cg_drawcrosshair 0\n\
cg_drawShellshock 0\n\
r_debugshader 0\n\
r_showfloatzdebug 0\n\
r_dof_tweak 0\n",
	};

	for (int i = 0; i < 12; i++) {

		std::string cfgPath = Dvar_FindVar("fs_homepath")->current.string
			+ std::string("\\main\\") + mvm_configs[i];

		if (!File_Exists(cfgPath))
		{
			std::ofstream cfgFile;
			
			cfgFile.open(cfgPath.c_str(), std::ios_base::app);
			cfgFile << mvm_configs_contents[i];
			cfgFile.close();
		}
	}
}

void InitializeAddresses()
{
	iw3_d3d9_device = *(IDirect3DDevice9**)FindPattern("68 ?? ?? ?? ?? 53 55 52 8B 15", 1);
	toggle_console.value = (uint32_t*)FindPattern("51 F6 05 ?? ?? ?? ?? ?? 53 55", 3);
	window_isActive = (uint32_t*)FindPattern("83 3D ?? ?? ?? ?? ?? 75 06 5F E9", 2);
	// TODO: find patterns for this boi (source engine does not display the accurate FOV but rather scaled)
	realFov_divisor = (float*)0x797610;

	auto g_viewInfo = FindPattern("81 C2 ?? ?? ?? ?? 81 C1 ?? ?? ?? ?? 89 90 ?? ?? ?? ?? 89 88 ?? ?? ?? ?? 39 35", 2);
	auto aaGlobals = FindPattern("81 C6 ?? ?? ?? ?? 6A 00 56 E8 ?? ?? ?? ?? 8B C3", 2);
	/* origin vector */

	player_right[0] = (float*)0x0CEF64DC;
	player_right[1] = (float*)0x0CEF64E0;
	player_right[2] = (float*)0x0CEF64E4;
	player_up[0] = (float*)0x0CEF64E8;
	player_up[1] = (float*)0x0CEF64EC;
	player_up[2] = (float*)0x0CEF64F0;
	if (gameVariant == GameVariant::COD4X)
	{
		//	temp fix
		player_pos[0] = (float*)0x797618;
		player_pos[1] = (float*)0x79761C;
		player_pos[2] = (float*)0x797620;
		player_lookat[0] = (float*)0x797624;
		player_lookat[1] = (float*)0x797628;
		player_lookat[2] = (float*)0x79762C;
	}
	else
	{
		player_pos[0] = (float*)0x72Af2C;
		player_pos[1] = (float*)0x72Af30;
		player_pos[2] = (float*)0x72Af34;
		player_lookat[0] = (float*)0x072AF44;
		player_lookat[1] = (float*)0x072AF48;
		player_lookat[2] = (float*)0x072AF4C;

	}

	/* roll :) (: */
	player_roll = (float*)FindPattern("D8 05 ?? ?? ?? ?? D9 1D ?? ?? ?? ?? 83 C4 14 C3", 2);

	/* how */
	auto R_UpdateFrontEndDvarOptions = FindPattern("C6 41 0B 00 C6 40 0B 00 FF D6 3B 05 ?? ?? ?? ?? 74 27");
	DebugShaderPatch = (BYTE*)(R_UpdateFrontEndDvarOptions + 7);
	FullBrightPatch = (BYTE*)(R_UpdateFrontEndDvarOptions + 3);

	*DebugShaderPatch = (BYTE)1;
	*FullBrightPatch = (BYTE)1;

	Dvar_RegisterString_Address = FindPattern("55 8B EC 83 E4 F8 8B 54 24 F4", 0, false);
	if (!Dvar_RegisterString_Address)
	{
		Dvar_RegisterString_Address = FindPattern("55 8B EC 83 E4 F8 83 EC 10 51 8B 4C 24 08 33 C0");
		Dvar_RegisterString_Version = 1;
	}

	Dvar_FindVar_Address = FindPattern("56 68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? A1 ?? ?? ?? ?? 85 C0 74 19 8B 35 ?? ?? ?? ?? EB 03 8D 49 00 6A 00 FF D6 8B 0D ?? ?? ?? ?? 85 C9 75 F2 8B C7", 0, false);
	if (!Dvar_FindVar_Address)
	{
		Dvar_FindVar_Address = FindPattern("56 57 68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? A1 ?? ?? ?? ?? 85 C0 74 18");
		Dvar_FindVar_Version = 1;
	}

	clc_demoPlaying_Address = *(uint32_t*)(FindPattern("51 53 56 8B F0 A1 ?? ?? ?? ?? 83 C0 80 3B F0 57 7F ?? 83 3D ?? ?? ?? ?? 00") + 20);
	clc_demoName_Address = FindPattern("68 ?? ?? ?? ?? 89 2D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 44 24 58", 1);
	cls_realtime_Address = (uint32_t*)0x956E98;
	clients_serverTime = (uint32_t*)FindPattern("A1 ?? ?? ?? ?? 0F 94 C2", 1);
	clients_snap_serverTime = (uint32_t*)FindPattern("8B 1D ?? ?? ?? ?? 89 1D ?? ?? ?? ??", 2);
	Con_TimeJumped_Call_Address = FindPattern("E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 8B 42 0C 83 C4 04 80 38 00");
	RewindPatch1_Address = FindPattern("79 0F 68 ?? ?? ?? ??");
	RewindPatch2_Address = FindPattern("7E 1C 50 51 68 ?? ?? ?? ?? 6A 0E");
	RewindPatch3_Address = FindPattern("75 09 8B C6 E8 ?? ?? ?? ?? EB 0F");
	clientConfigNumCOD4X = (uint32_t*)FindPattern("89 04 BD ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 83 EC 10", 3);
	cg_teamScores_axis = (uint32_t*)FindPattern("A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 8D 80 ?? ?? ?? ?? 83 C4 04", 1);
	cg_teamScores_allies = cg_teamScores_axis + 1;
	cg_landTime = (uint32_t*)FindPattern("A3 ?? ?? ?? ?? DE E1 D9 1D ?? ?? ?? ?? 5F 5E 5D", 1);
	clients_parseEntitiesNum = (uint32_t*)FindPattern("A1 ?? ?? ?? ?? 2B 83 ?? ?? ?? ?? 3D", 1);
	clients_parseClientsNum = clients_parseEntitiesNum + 1;
	clients_snapshots_Address = FindPattern("81 C3 ?? ?? ?? ?? 83 3B 00", 2);
	clients_parseEntities_Address = FindPattern("81 C6 ?? ?? ?? ?? 83 C0 01 B9 3D 00 00 00", 2);
	clients_parseClients_Address = FindPattern("81 C6 ?? ?? ?? ?? 83 C0 01 B9 19 00 00 00", 2);
	cgs_teamChatMessages_Address = FindPattern("81 C6 ?? ?? ?? ?? 74 30 80 3E 5E", 2);
	cg_Address = FindPattern("B8 ?? ?? ?? ?? 5F 5E 5D 5B 83 C4 50", 1);
	cg_serverCommandNum = (uint32_t*)(cg_Address + 0x230A0);
	cgs_serverCommandSequence = (uint32_t*)FindPattern("89 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8", 2);
	clc_serverCommandSequence = (uint32_t*)FindPattern("8B 3D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 35 ?? ?? ?? ?? 03 F3 3B 35", 2);
	clc_timeDemoFrames = (uint32_t*)FindPattern("DB 05 ?? ?? ?? ?? A1 ?? ?? ?? ?? DD 05", 2);
	cgs_processedSnapshotNum = (uint32_t*)FindPattern("A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8D 90", 1);
	cg_latestSnapshotNum = (uint32_t*)FindPattern("A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8D 90", 7);
	fileData_Address = FindPattern("83 BE ?? ?? ?? ?? ?? C7 86 ?? ?? ?? ?? ?? ?? ?? ?? 75 02 33 ED", 2);

	uint32_t FS_Read_Call_Address = FindPattern("E8 ?? ?? ?? ?? 83 C4 0C 53 8D 44 24 5C");
	uint32_t FS_Read_Original_Address = *((uint32_t*)(FS_Read_Call_Address + 0x1)) + FS_Read_Call_Address + 0x5;
	if (gameVariant == GameVariant::COD4X)
		FS_Read_Address = *((uint32_t*)(FS_Read_Original_Address + 0x1)) + FS_Read_Original_Address + 0x5;
	else
		FS_Read_Address = FS_Read_Original_Address;

	uint32_t CL_FirstSnapshot_Call_Address = FindPattern("E8 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ?? 0F 85");
	CL_FirstSnapshot_Address = *((uint32_t*)(CL_FirstSnapshot_Call_Address + 0x1)) + CL_FirstSnapshot_Call_Address + 0x5;

	// actually RB_UpdateLogging since its after RB_SwapBuffers
	RB_SwapBuffersHook_Address = FindPattern("51 8B 15 ?? ?? ?? ?? 8B 42 0C 85 C0");

	if (gameVariant == GameVariant::Stock) 
	{
		CL_CGameNeedsServerCommandPatch_Address = FindPattern("3B 35 ?? ?? ?? ?? 7E 0F") + 6;
		CL_RunOncePerClientFramePatch_Address = FindPattern("68 ?? ?? ?? ?? 6A 00 6A 00 E8 ?? ?? ?? ?? 83 C4 0C 8B 15");
		Demo_IsPaused_Address = FindPattern("01 35 ?? ?? ?? ?? 8B C1");
	}
	else 
	{
		CL_CGameNeedsServerCommandPatch_Address = FindPattern("B8 E0 4C 8F 00 8B 80 40 01 02 00 ?? ?? ?? ?? ?? C7", 0, true, cod4xModuleName.c_str()) + 14;
		CL_RunOncePerClientFramePatch_Address = FindPattern("C7 44 24 08 ?? ?? ?? ?? C7 44 24 04 00 00 00 00 C7 04 24 00 00 00 00 E8 ?? ?? ?? ?? A1", 0, true, cod4xModuleName.c_str());
		Demo_IsPaused_Address = FindPattern("B8 80 6D 95 00 8B 90 18 01 00 00 B9 80 6D 95 00", 0, true, cod4xModuleName.c_str());
	}

	CL_Disconnect_Address = FindPattern("55 8B EC 83 E4 F8 83 EC 14 53 33 DB 38 1D", 0, false);
	if (!CL_Disconnect_Address) 
	{
		// TODO: Change this since this overwrites the COD4X hook
		uint32_t CL_Disconnect_Fallback = 0x4696B0;
		if (*(uint8_t*)CL_Disconnect_Fallback == 0xE9) 
		{
			// Was hooked, (most likely) by cod4x
			CL_Disconnect_Address = CL_Disconnect_Fallback;
		}
	}

	UI_TextWidth_Internal_Address = FindPattern("51 D9 44 24 10 50");
	R_AddCmdDrawStretchPic_Address = FindPattern("8B 15 ?? ?? ?? ?? 53 8B 5C 24 28 56 57 8B F8 85 FF 8B F7 75 02 8B F2 8B 4E 40 8B 41 08 83 78 28 00 75 06 83 78 20 00 74 1F 8B 46 44 3B 42 44 75 0D 8B 46 48 3B 42 48 75 05 3B 4A 40 74 0A 8B 0F 51 68 ?? ?? ?? ?? EB 0E F6 46 3D 10 74 18 8B 17 52 68 ?? ?? ?? ?? 6A 08 E8 ?? ?? ?? ?? 8B 35 ?? ?? ?? ?? 83 C4 0C 8B 0D ?? ?? ?? ?? 8B 51 04 8B 41 08 8B 3D ?? ?? ?? ?? 2B C2 8D 84 38 ?? ?? ?? ?? 83 F8 2C 7D 0B 5F 5E C7 41 ?? ?? ?? ?? ?? 5B C3 8B 01 D9 44 24 10 03 C2 89 41 0C 83 C2 2C 85 DB 89 51 04 66 C7 00 ?? ?? 66 C7 40 ?? ?? ?? D9 58 08 D9 44 24 14 89 70 04 D9 58 0C 8D 50 28 D9 44 24 18 D9 58 10 D9 44 24 1C D9 58 14 D9 44 24 20 D9 58 18 D9 44 24 24 D9 58 1C D9 44 24 28 D9 58 20 D9 44 24 2C D9 58 24 75 0A 5F 5E C7 02 ?? ?? ?? ?? 5B C3 5F 5E 8B CB 5B E9");
	R_AddCmdDrawText_Address = FindPattern("8B 44 24 04 80 38 00 0F 84");

	CL_RegisterFont = (CL_RegisterFont_t)FindPattern("B8 ?? ?? ?? ?? 75 05 B8 ?? ?? ?? ?? 57 68 ?? ?? ?? ?? FF D0 8B 0D", 1);
	Material_RegisterHandle = (Material_RegisterHandle_t)FindPattern("B8 ?? ?? ?? ?? 75 05 B8 ?? ?? ?? ?? 6A 07 68 ?? ?? ?? ?? FF D0 A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 83 C4 08 80 78 0C 00 B8 ?? ?? ?? ?? 75 05 B8 ?? ?? ?? ?? 57", 1);
	CG_Draw2D_Address = FindPattern("55 8B EC 83 E4 F8 83 EC 1C 83 3D ?? ?? ?? ?? ?? 53 56 57 8B F8");

	R_SetViewParmsForScene_Address = FindPattern("51 57 68 ?? ?? ?? ?? 8B F8");
	AnglesToAxisCall1_Address = FindPattern("51 8B 0D ?? ?? ?? ?? D9 EE") + 0x2FD;
	AnglesToAxisCall2_Address = FindPattern("DC 0D ?? ?? ?? ?? 8D 74 24 3C") + 14;
	FX_SetupCamera_Address = FindPattern("55 8B EC 83 E4 F8 83 EC 08 D9 46 18");
	CG_CalcFov_Address = FindPattern("55 8B EC 83 E4 F8 83 EC 08 D9 45 08 DC 0D");
	CG_DObjGetWorldTagMatrix_Address = FindPattern("83 EC 34 53 8B 5C 24 40 55 8B 6C 24 48");

	AnglesToAxis_Address = FindPattern("83 EC 24 D9 46 04");
	viewanglesX_Address = FindPattern("D9 05 ?? ?? ?? ?? 57 8B F8", 2);
	viewanglesY_Address = viewanglesX_Address + 4;

	refdef = (refdef_t*)FindPattern("BE ?? ?? ?? ?? D9 1C 24 E8 ?? ?? ?? ?? 8D 44 24 18", 1);
	cmd_arg_id = (uint32_t*)FindPattern("8B 0D ?? ?? ?? ?? 8B 04 8D ?? ?? ?? ?? 81 EC ?? ?? ?? ?? 83 F8 02 53", 2);
	cmd_argc = (uint32_t*)FindPattern("8B 04 8D ?? ?? ?? ?? 81 EC ?? ?? ?? ?? 83 F8 02 53", 3);
	cmd_argv = (uint32_t**)FindPattern("8B 04 8D ?? ?? ?? ?? 8B 00 EB 05 B8 ?? ?? ?? ?? 50 68 ?? ?? ?? ?? 6A 0E", 3);
	cmd_functions = (cmd_function_t**)FindPattern("83 3D ?? ?? ?? ?? ?? BF ?? ?? ?? ?? 74 24", 2);

	Cbuf_AddText_Address = FindPattern("55 56 57 68 ?? ?? ?? ?? 8B F0");
	Cmd_ExecuteSingleCommand_Address = FindPattern("A1 ?? ?? ?? ?? 85 C0 53 55 8B 6C 24 14");
	Dvar_RegisterColor_Address = FindPattern("83 EC 20 D9 44 24 28", 0, false);
	if (!Dvar_RegisterColor_Address)
	{
		Dvar_RegisterColor_Address = FindPattern("83 EC 24 D9 44 24 28");
		Dvar_RegisterColor_Version = 1;
	}

	CL_KeyEvent_Address = FindPattern("8B 44 24 0C 81 EC ?? ?? ?? ?? 53");
	R_TakeScreenshot = (R_TakeScreenshot_t)FindPattern("55 8B EC 83 E4 F8 83 EC 54 53 8B 5D 0C");
#ifdef IW3D_EXPORT
	SL_ConvertToString_Address = FindPattern("8B 15 ?? ?? ?? ?? 8D 0C 40 84 5C 8A 02", 2);
#endif
	Con_IsVisible_Address = FindPattern("F6 05 ?? ?? ?? ?? ?? 53 55", 2);
}

void DetermineGameVariant() 
{
	gameVariant = GameVariant::Stock;

	HANDLE moduleSnapshot = INVALID_HANDLE_VALUE;

	moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (moduleSnapshot == INVALID_HANDLE_VALUE)
	{
		Console::Log(RED, "Failed to determine game variant.\nAssuming stock COD4...");
		return;
	}

	MODULEENTRY32 moduleEntry;
	moduleEntry.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(moduleSnapshot, &moduleEntry))
	{
		Console::Log(RED, "Failed to determine game variant.\nAssuming stock COD4...");
		return;
	}

	do
	{
		if (std::string(moduleEntry.szModule).find("cod4x") != std::string::npos) 
		{
			gameVariant = GameVariant::COD4X;
			cod4xModuleName = std::string(moduleEntry.szModule);
		}

	} while (Module32Next(moduleSnapshot, &moduleEntry));

	CloseHandle(moduleSnapshot);
}

void Game_Init()
{
	DetermineGameVariant();

	if (gameVariant == GameVariant::COD4X) 
	{
		UnprotectModule(cod4xModuleName.c_str());
	}

	InitializeAddresses();

	GenerateDefaultConfigs();

	timescale = Dvar_FindVar("timescale");
	cg_draw2D = Dvar_FindVar("cg_draw2D");
	cg_fov = Dvar_FindVar("cg_fov");
	cg_fovscale = Dvar_FindVar("cg_fovscale");
	com_maxfps = Dvar_FindVar("com_maxfps");

	cg_fov->domain.value.min = 5;
	cg_fov->domain.value.max = 160;

	Dvar_FindVar("com_maxframetime")->domain.integer.min = 10;

	Dvar_FindVar("sv_cheats")->current.enabled = true;

	ExtendLODDistance();


	/* REGISTER CUSTOM DVARS */

	mvm_fog_color = Dvar_RegisterColor("mvm_fog_color", 0.7f, 1.0f, 0.1f, 1.0f, 0x100, "Custom color to use for fog");
	mvm_fog_custom = Dvar_RegisterBool("mvm_fog_custom", 0);
	mvm_fog_start = Dvar_RegisterFloat("mvm_fog_start", 500, 0, 50000);
	mvm_fog_end = Dvar_RegisterFloat("mvm_fog_end", 1200, 0, 50000);
	
	mvm_dolly_frozen = Dvar_RegisterBool("mvm_dolly_frozen", 0);
	mvm_dolly_skip = Dvar_RegisterBool("mvm_dolly_skip", 1);
	mvm_dolly_lmb = Dvar_RegisterBool("mvm_dolly_lmb", 0);
	mvm_dolly_linear = Dvar_RegisterBool("mvm_dolly_linear", 0);
	mvm_dolly_frozen_speed = Dvar_RegisterFloat("mvm_dolly_frozen_speed", 25.f, 1.0f, 100.f);
	mvm_dolly_frozen_speed->description = "Base frozen dolly cam speed, this amount will be multiplied by the current timescale\n";
	mvm_cam_speed = Dvar_RegisterFloat("mvm_cam_speed", 1, 0.01f, 2.0f);

	mvm_killfeed_custom = Dvar_RegisterBool("mvm_killfeed_custom", 0);
	mvm_killfeed_speed = Dvar_RegisterInt("mvm_killfeed_speed", 100, 1, 500);

	mvm_forcelod = Dvar_RegisterBool("mvm_forcelod", 1);

#ifdef IW3D_EXPORT
	iw3d_fps = Dvar_RegisterInt("iw3d_fps", 0, 0, 1000);
#else

	is_recording_streams = false;
	mvm_streams_fps = Dvar_RegisterInt("mvm_streams_fps", 0, 0, 1000);
	mvm_streams_fps->description = "Framerate for multi-pass recording (see mvm_streams_passes)\n\
Value MUST be a whole fraction of 1000, will be converted to lower fraction otherwise\n\
Press R to start/stop recording\n";

	is_recording_avidemo = false;
	mvm_avidemo_fps = Dvar_RegisterInt("mvm_avidemo_fps", 0, 0, 1000);
	mvm_avidemo_fps->description = "Record a set of images (or video), input MUST be a whole fraction of 1000. Press R to stop recording\n";

	mvm_streams_aeExport = Dvar_RegisterBool("mvm_streams_aeExport", false);
	mvm_streams_aeExport->description = "Export a campath for camera tracking in After Effects";

	mvm_streams_depthFormat = Dvar_RegisterInt("mvm_streams_depthFormat", 0, 0, 2);
	mvm_streams_depthFormat->description = "zDepth output:\n\
Options:\n\
  0 - Greyscale baked (Standard, most common)\n\
  1 - Default (Cod4 Method - packed)\n\
  2 - HSV Multi channel (Accurate, less common)\n";

	mvm_streams_aeExport_sun = Dvar_RegisterBool("mvm_streams_aeExport_sun", 0);
	mvm_streams_aeExport_sun->description = "When mvm_streams_aeExport is set to 1, this variable can be used to export the coordinates of the sun\n\
This is done in accordance to r_lightTweakSunDirection";
	mvm_streams_passes = Dvar_RegisterString("mvm_streams_passes", "mvm_w-g mvm_w-gd mvm_gG");
	mvm_streams_passes->description = "Streams passes as config files (without their .cfg suffix), loaded from the /main/ directory\n\
It is possible to add your own configs as custom passes. The default configs are:\n\
mvm_w       -   World pass (w = world)\n\
mvm_wd      -   Depth pass (d = depth)\n\
mvm_wn      -   Normals pass (n = normals), This does not mean 'generic' or 'regular' pass\n\
mvm_w-g     -   World pass minus gun (-g = minus gun)\n\
mvm_w-gd    -   Depth pass minus gun\n\
mvm_w-gn    -   Normals pass minus gun\n\
mvm_gn      -   Gun + Normals pass\n\
mvm_gG      -   Gun + Green background\n\
mvm_gB      -   Gun + Blue background\n\
mvm_gR      -   Gun + Red background\n\
mvm_gA      -   Gun (Black) with a solid white background (For luma mattes)\n\
mvm_hud     -   Hud-only\n";

	mvm_export_format = Dvar_RegisterString("mvm_export_format", "avi");
	mvm_export_format->description = "Recording output format (tga or avi)\n";

	mvm_streams_interval = Dvar_RegisterInt("mvm_streams_interval", 1, 1, 5);
	mvm_streams_interval->description = "The amount of frames to skip during the recording process.\n\
Due to GPU/CPU Synchronization differences, applying configs may take up more frames than it takes to screenshot.\n\
In case of incorrect streams, you can alter / increment this value to a higher count.\n\
This variable sacrifices frames for better rendering performance, keep in mind that this slows down the time to render\n";

	mvm_output_directory = Dvar_RegisterString("mvm_output_directory", "");
	mvm_output_directory->description = "Directory to save your streams files to\n\
To reset, enter 'default' without quotes";
	//mvm_streams_fileName = Dvar_RegisterString("mvm_streams_fileName", "");
#endif

	/* REGISTER CUSTOM COMMANDS */

	cmd_function_t* cmd = Cmd_AddCommand("mvm_cam_import", CameraImport);
	cmd->autoCompleteDir = campathExportDirectory;
	cmd->autoCompleteExt = campathExportExtension;

	cmd = Cmd_AddCommand("mvm_cam_export", CameraExport);
	cmd->autoCompleteDir = campathExportDirectory;
	cmd->autoCompleteExt = campathExportExtension;


	// hook Cmd_ExecuteSingleCommand to handle certain dvar changes
	Cmd_ExecuteSingleCommand_tramp = Hook<void(*)(int, int, const char*)>(Cmd_ExecuteSingleCommand_Address, hkCmd_ExecuteSingleCommand);
}