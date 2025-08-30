#pragma once

enum class GameVariant {
	Stock, COD4X
};

extern GameVariant gameVariant;

extern void Game_Init();

extern dvar_s* Dvar_FindVar(std::string dvar);


// a few frequently used dvars
extern dvar_s* timescale;
extern dvar_s* cg_draw2D;
extern dvar_s* cg_fov;
extern dvar_s* cg_fovscale;
extern dvar_s* com_maxfps;

// custom dvars
extern dvar_s* mvm_fog_color;
extern dvar_s* mvm_fog_custom;
extern dvar_s* mvm_fog_start;
extern dvar_s* mvm_fog_end;

extern dvar_s* mvm_cam_speed;

extern dvar_s* mvm_dolly_linear;
extern dvar_s* mvm_dolly_frozen;
extern dvar_s* mvm_dolly_skip;
extern dvar_s* mvm_dolly_lmb;
extern dvar_s* mvm_dolly_frozen_speed;

extern dvar_s* mvm_killfeed_custom;
extern dvar_s* mvm_killfeed_speed;

extern dvar_s* mvm_forcelod;

extern bool is_recording_streams;
extern dvar_s* mvm_streams_fps;
extern dvar_s* mvm_streams_passes;
extern dvar_s* mvm_streams_aeExport;
extern dvar_s* mvm_streams_pixfmt;
extern dvar_s* mvm_streams_aeExport_sun;

extern bool is_recording_avidemo;
extern dvar_s* mvm_avidemo_fps;

extern dvar_s* mvm_export_format;
extern dvar_s* mvm_streams_depthFormat;
extern dvar_s* mvm_streams_interval;

extern dvar_s* mvm_output_directory;
extern std::string standard_output_directory;

extern dvar_s* iw3d_fps;

struct refdef_t
{
	int windowPosition[2];
	int windowSize[2];
	float tanHalfFovX;
	float tanHalfFovY;
	vec3_t position;
	vec3_t viewmatrix[3];
	// ...
};

extern refdef_t* refdef;

typedef struct addressValue_s
{
	uint32_t* value;
	uint32_t  original;
}	addressValue_t;


struct cmd_function_t
{
	cmd_function_t* next;
	const char* name;
	const char* autoCompleteDir;
	const char* autoCompleteExt;
	void(*function)(void);
};

extern void Cbuf_AddText(std::string command);
extern void Cmd_ExecuteSingleCommand(std::string command);

extern int Cmd_Argc(void);

extern char* Cmd_Argv(int arg);

extern IDirect3DDevice9* iw3_d3d9_device;
extern addressValue_t		toggle_console;
extern uint32_t*			window_isActive;
extern float* realFov_divisor;

extern float* player_pos[3];
extern float* player_lookat[3];
extern float* player_right[3];
extern float* player_up[3];

extern float* player_roll;

extern uint32_t clc_demoPlaying_Address;
extern uint32_t Demo_IsPaused_Address;
extern uint32_t clc_demoName_Address;
extern uint32_t* cls_realtime_Address;
extern uint32_t* clients_snap_serverTime;
extern uint32_t GfxDrawSceneFullbright;
extern uint32_t GfxDrawSceneDebugShader;
extern uint32_t* clients_serverTime;
extern uint32_t* clientConfigNumCOD4X;
extern uint32_t* cg_teamScores_axis;
extern uint32_t* cg_teamScores_allies;
extern uint32_t* cg_landTime;
extern uint32_t* clients_parseEntitiesNum;
extern uint32_t* clients_parseClientsNum;
extern uint32_t clients_snapshots_Address;
extern uint32_t clients_parseEntities_Address;
extern uint32_t clients_parseClients_Address;
extern uint32_t cgs_teamChatMessages_Address;
extern uint32_t cg_Address;
extern uint32_t* cg_serverCommandNum;
extern uint32_t* cgs_serverCommandSequence;
extern uint32_t* clc_serverCommandSequence;
extern uint32_t* clc_timeDemoFrames;
extern uint32_t* cgs_processedSnapshotNum;
extern uint32_t* cg_latestSnapshotNum;
extern uint32_t fileData_Address;

extern uint32_t RB_SwapBuffersHook_Address;
extern uint32_t CL_RunOncePerClientFramePatch_Address;
extern uint32_t CL_Disconnect_Address;
extern uint32_t Dvar_RegisterString_Address;
extern uint32_t Con_TimeJumped_Call_Address;
extern uint32_t FS_Read_Address;
extern uint32_t RewindPatch1_Address;
extern uint32_t RewindPatch2_Address;
extern uint32_t RewindPatch3_Address;
extern uint32_t CL_FirstSnapshot_Address;
extern uint32_t CL_CGameNeedsServerCommandPatch_Address;

extern uint32_t UI_TextWidth_Internal_Address;
extern uint32_t R_AddCmdDrawStretchPic_Address;
extern uint32_t R_AddCmdDrawText_Address;
