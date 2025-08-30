#include "stdafx.hpp"
#include "Demo.hpp"
#include "Utility.hpp"
#include "Game.hpp"
#include "Misc.hpp"
#include "Drawing.hpp"
#include "IW3D.hpp"
#include "Streams.hpp"
#include "IW3D.hpp"
#include "Dollycam.hpp"
#include "Console.hpp"

uint32_t demoStartTick = -1;
uint32_t demoEndTick = -1;

uint32_t clc_demoPlaying_Address;
uint32_t Demo_IsPaused_Address;
uint32_t clc_demoName_Address;
uint32_t* cls_realtime_Address;
uint32_t* clients_snap_serverTime;
uint32_t* clients_serverTime;
uint32_t* clientConfigNumCOD4X;
uint32_t* cg_teamScores_axis;
uint32_t* cg_teamScores_allies;
uint32_t* cg_landTime;
uint32_t* clients_parseEntitiesNum;
uint32_t* clients_parseClientsNum;
uint32_t clients_snapshots_Address;
uint32_t clients_parseEntities_Address;
uint32_t clients_parseClients_Address;
uint32_t cgs_teamChatMessages_Address;
uint32_t cg_Address;
uint32_t* cg_serverCommandNum;
uint32_t* cgs_serverCommandSequence;
uint32_t* clc_serverCommandSequence;
uint32_t* clc_timeDemoFrames;
uint32_t* cgs_processedSnapshotNum;
uint32_t* cg_latestSnapshotNum;
uint32_t fileData_Address;

uint32_t RB_SwapBuffersHook_Address;
uint32_t CL_RunOncePerClientFramePatch_Address;
uint32_t CL_Disconnect_Address;
uint32_t Con_TimeJumped_Call_Address;
uint32_t FS_Read_Address;
uint32_t RewindPatch1_Address;
uint32_t RewindPatch2_Address;
uint32_t RewindPatch3_Address;
uint32_t CL_FirstSnapshot_Address;
uint32_t CL_CGameNeedsServerCommandPatch_Address;

std::string current_dir;
uint32_t	fileNum;

enum class FileStreamState {
	Unitialized,
	Ready,
	InitializationFailed
};

FileStreamState fileStreamState = FileStreamState::Unitialized;
std::ifstream currentDemoFile;
int currentDemoFileOffset = 0;

std::stack<CustomSnapshot> snapshotData;
int snapshotCount = 0;

std::uint32_t snapshotIntervalTime = 2500;
bool rewindDemo = false;

bool Demo_IsPlaying()
{
	return *(int*)clc_demoPlaying_Address;
}

bool Demo_IsPaused()
{
	return (*(BYTE*)Demo_IsPaused_Address) == 0x90;
}

std::string Demo_GetName()
{
	std::string name((char*)clc_demoName_Address);

	return name;
}

std::string Demo_GetNameEscaped()
{
	std::string name((char*)clc_demoName_Address);

	std::string result = "";
	for (char c : name)
	{
		if (c == '/' || c == '\\' || c == ':')
			result += '_';
		else
			result += c;
	}

	return result;
}

void Demo_TogglePause()
{
	static uint8_t originalCode[32];

	uint32_t patchSize = gameVariant == GameVariant::Stock ? 6 : 27;
	if (!Demo_IsPaused())
	{
		for (size_t i = 0; i < patchSize; i++) 
		{
			originalCode[i] = *(uint8_t*)(Demo_IsPaused_Address + i);
			*(uint8_t*)(Demo_IsPaused_Address + i) = 0x90;
		}
	}
	else
	{
		for (size_t i = 0; i < patchSize; i++)
		{
			*(uint8_t*)(Demo_IsPaused_Address + i) = originalCode[i];
		}
	}
}

uint32_t Demo_GetCurrentTick()
{
	return *clients_serverTime;
}

uint32_t Demo_GetStartTick()
{
	return demoStartTick;
}

uint32_t Demo_GetEndTick()
{
	return demoEndTick;
}

void Demo_SkipForward(uint32_t amount)
{
	*cls_realtime_Address += amount;
}

void RestoreOldGamestate();

void Demo_SkipBack()
{
	rewindDemo = true;

	// Not sure if this is actually a good idea but it seems to work
	if (Demo_IsPaused())
		Demo_SkipForward(200);
}

// Skips back to beginning of demo
void Demo_Rewind()
{
	while (snapshotData.size() > 1)
		snapshotData.pop();

	Demo_SkipBack();
}

void Recording_Frame()
{
	if (recordingStatus == RecordingStatus::Finished)
	{
		Reset_Recording();
	}

	while (is_recording_streams && (1000 % mvm_streams_fps->current.integer != 0))
		mvm_streams_fps->current.integer -= 1;

	while (is_recording_avidemo && (1000 % mvm_avidemo_fps->current.integer != 0))
		mvm_avidemo_fps->current.integer -= 1;

	if (is_recording_streams)
	{
		Streams_Frame();
	} 
	else if (is_recording_avidemo)
	{
		Avidemo_Frame();
	}
}

uint32_t RB_SwapBuffers_tramp;
void RB_SwapBuffers()
{
	if (Demo_IsPlaying())
	{

		frameCount++;

#ifdef IW3D_EXPORT
		if (iw3d_fps->current.integer)
			IW3D_Frame();
		else if (Dvar_FindVar("cl_avidemo")->current.integer)
			Screenshot_cod4(va("%s\\%s\\screenshots\\%s\\%i.tga", Dvar_FindVar("fs_homepath")->current.string, Dvar_FindVar("fs_game")->current.string, Demo_GetNameEscaped().c_str(), frameCount));
#else
		Recording_Frame();

		if (Dvar_FindVar("cl_avidemo")->current.integer)
			Screenshot_cod4(va("%s\\%s\\screenshots\\%s\\%i.tga", Dvar_FindVar("fs_homepath")->current.string, Dvar_FindVar("fs_game")->current.string, Demo_GetNameEscaped().c_str(), frameCount));
#endif

		if (Demo_GetCurrentTick() > Demo_GetEndTick())
		{
			demoEndTick = Demo_GetCurrentTick();
		}

		if (mvm_forcelod->current.enabled)
		{
			ExtendLODDistance();
		}
	}
}

void __declspec(naked) hkRB_SwapBuffers()
{
	__asm pushad

	RB_SwapBuffers();

	__asm popad
	__asm jmp RB_SwapBuffers_tramp
}

void(*CL_Disconnect_tramp)(std::int32_t);
void hkCL_Disconnect(std::int32_t a1)
{
	// reset these since we're not in a demo anymore
	demoStartTick = -1;
	demoEndTick = -1;

#ifdef IW3D_EXPORT
	Cmd_ExecuteSingleCommand("iw3d_fps 0");
#else
	is_recording_streams = false;
	is_recording_avidemo = false;
#endif

	currentDemoFile.close();
	fileStreamState = FileStreamState::Unitialized;
	rewindDemo = false;
	snapshotCount = 0;
	currentDemoFileOffset = 0;
	snapshotData = std::stack<CustomSnapshot>{};

	CL_Disconnect_tramp(a1);
}

void RestoreOldGamestate()
{
	if (snapshotData.size() > 0) 
	{
		if (snapshotData.size() > 1 && Demo_GetCurrentTick() - snapshotData.top().serverTime < snapshotIntervalTime * 0.5f)
			snapshotData.pop();

		memcpy((void*)cgs_teamChatMessages_Address, &snapshotData.top().chat, sizeof(CustomSnapshot{}.chat));
		*cg_serverCommandNum = snapshotData.top().serverCommandNum;
		*cgs_serverCommandSequence = snapshotData.top().serverCommandSequence1;
		*clc_serverCommandSequence = snapshotData.top().serverCommandSequence2;

		memcpy((void*)clients_snapshots_Address, &snapshotData.top().snapshots, sizeof(CustomSnapshot{}.snapshots));
		memcpy((void*)clients_parseEntities_Address, &snapshotData.top().entities, sizeof(CustomSnapshot{}.entities));
		memcpy((void*)clients_parseClients_Address, &snapshotData.top().clients, sizeof(CustomSnapshot{}.clients));

		*cg_landTime = snapshotData.top().landTime;
		*cg_teamScores_axis = snapshotData.top().axisScore;
		*cg_teamScores_allies = snapshotData.top().alliesScore;

		*clients_parseEntitiesNum = snapshotData.top().parseEntitiesNum;
		*clients_parseClientsNum = snapshotData.top().parseClientsNum;

		*clients_snap_serverTime = snapshotData.top().serverTime;		// cl.snap.serverTime
		*clients_serverTime = 0;										// cl.serverTime
		*(clients_serverTime + 1) = 0;									// cl.oldServerTime
		*(clients_serverTime + 3) = 0;									// cl.serverTimeDelta

		*clc_timeDemoFrames = 0;										// clc.timeDemoFrames
		*(clc_timeDemoFrames + 1) = 0;									// clc.timeDemoStart
		*(clc_timeDemoFrames + 2) = 0;									// clc.timeDemoBaseTime
		*(clc_timeDemoFrames + 3) = 0;									// cls.realtime
		*(clc_timeDemoFrames + 4) = 0;									// cls.realFrametime

		*cgs_processedSnapshotNum = 0;
		*cg_latestSnapshotNum = 0;										// cg_t -> latestSnapshotNum
		*(cg_latestSnapshotNum + 1) = 0;								// cg_t -> latestSnapshotTime
		*(cg_latestSnapshotNum + 2) = 0;								// cg_t -> snap
		*(cg_latestSnapshotNum + 3) = 0;								// cg_t -> nextSnap

		*clc_serverCommandSequence = 0;									// command string parseNum
		*(clc_serverCommandSequence + 1) = 0;							// command string parseNum2 ?
		*clientConfigNumCOD4X = snapshotData.top().clientConfigNumCoD4X;	// only needed for CoD4X demos, client config data parseNum

		int clientNum = *(uint32_t*)cg_Address;

		// NOP call to Con_TimeJumped since that produces a DivByZero exception
		unsigned char Con_TimeJumped_Call[5];
		if (gameVariant == GameVariant::Stock)
		{
			memcpy(Con_TimeJumped_Call, (void*)Con_TimeJumped_Call_Address, sizeof(Con_TimeJumped_Call));

			for (int i = 0; i < 5; i++)
				*(BYTE*)(Con_TimeJumped_Call_Address + i) = 0x90;
		}

		_asm {
			pushad
			mov eax, clientNum
			call CL_FirstSnapshot_Address
			popad
		}

		// Restore call again
		if (gameVariant == GameVariant::Stock)
		{
			memcpy((void*)Con_TimeJumped_Call_Address, Con_TimeJumped_Call, sizeof(Con_TimeJumped_Call));
		}

		rewindDemo = false;
		currentDemoFileOffset = snapshotData.top().fileOffset;
		currentDemoFile.seekg(currentDemoFileOffset);
	}
}

void StoreCurrentGamestate()
{
	if (snapshotCount == 3) 
	{
		// initially set start tick and end tick
		demoStartTick = Demo_GetCurrentTick();
		demoEndTick = demoStartTick + 10000;
	}

	if (!snapshotData.empty() && snapshotData.top().fileOffset && !snapshotData.top().serverTime) 
	{
		snapshotData.top().clientConfigNumCoD4X = *clientConfigNumCOD4X;
		snapshotData.top().axisScore = *cg_teamScores_axis;
		snapshotData.top().alliesScore = *cg_teamScores_allies;

		snapshotData.top().landTime = *cg_landTime;
		snapshotData.top().serverTime = *clients_snap_serverTime;
		snapshotData.top().parseEntitiesNum = *clients_parseEntitiesNum;
		snapshotData.top().parseClientsNum = *clients_parseClientsNum;

		memcpy(&snapshotData.top().snapshots, (void*)clients_snapshots_Address, sizeof(CustomSnapshot{}.snapshots));
		memcpy(&snapshotData.top().entities, (void*)clients_parseEntities_Address, sizeof(CustomSnapshot{}.entities));
		memcpy(&snapshotData.top().clients, (void*)clients_parseClients_Address, sizeof(CustomSnapshot{}.clients));

		memcpy(&snapshotData.top().chat, (void*)cgs_teamChatMessages_Address, sizeof(CustomSnapshot{}.chat));
		snapshotData.top().serverCommandNum = *cg_serverCommandNum;
		snapshotData.top().serverCommandSequence1 = *cgs_serverCommandSequence;
		snapshotData.top().serverCommandSequence2 = *clc_serverCommandSequence;
	}

	if (snapshotCount >= 3 && (snapshotData.empty() || *clients_snap_serverTime >= snapshotData.top().serverTime + snapshotIntervalTime))
	{
		snapshotData.emplace();
		snapshotData.top().fileOffset = currentDemoFileOffset - 9;
	}
}

void FS_LoadDemoFile(char* fileName)
{
	std::string basePath = Dvar_FindVar("fs_basepath")->current.string;
	const char* modPath = Dvar_FindVar("fs_game")->current.string;

	std::vector<std::string> searchPaths;
	if (fileName[1] == ':')
	{
		searchPaths.push_back(fileName);
	}
	else
	{
		if (strlen(modPath) >= 2)
			searchPaths.push_back(basePath + "\\" + static_cast<std::string>(modPath) + "\\" + static_cast<std::string>(fileName));
		searchPaths.push_back(basePath + "\\main\\" + static_cast<std::string>(fileName));
		searchPaths.push_back(basePath + "\\players\\" + static_cast<std::string>(fileName));
	}

	for (std::string path : searchPaths)
	{
		currentDemoFile.open(path.c_str(), std::ios::binary);

		if (currentDemoFile.is_open())
			break;
	}

	if (currentDemoFile.is_open())
	{
		fileStreamState = FileStreamState::Ready;
	}
	else
	{
		fileStreamState = FileStreamState::InitializationFailed;
		Console::Log(RED, "Failed to open demo file! Rewinding is disabled.");
	}
}


typedef int(__cdecl * FS_Read_t)(void* buffer, int len, int fileHandleIndex);
FS_Read_t FS_Read_tramp;

int hkFS_Read(void* buffer, int len, int fileHandleIndex)
{
	fileHandleData_t fileHandle = *reinterpret_cast<fileHandleData_t*>(fileData_Address + fileHandleIndex * sizeof(fileHandleData_t));

	if (std::string(fileHandle.name).find(".dm_1") == std::string::npos)
		return FS_Read_tramp(buffer, len, fileHandleIndex);

	if (fileStreamState == FileStreamState::Unitialized) 
		FS_LoadDemoFile(fileHandle.name);

	if (fileStreamState != FileStreamState::Ready)
		return FS_Read_tramp(buffer, len, fileHandleIndex);

	if (len == 1 && rewindDemo)
		RestoreOldGamestate();
	else if (len > 12) {
		snapshotCount++;

		StoreCurrentGamestate();
	}

	currentDemoFile.read(reinterpret_cast<char*>(buffer), len);
	currentDemoFileOffset += len;

	if (currentDemoFileOffset != currentDemoFile.tellg())
	{
		Console::Log(RED, "ERROR: currentDemoFileOffset != currentDemoFile.tellg()");
	}
	return len;
}

void Patch_Demo()
{
	// hook RB_SwapBuffers (d3d9 present) so we can do things per-frame
	// hook is actually *after* RB_SwapBuffers
	RB_SwapBuffers_tramp = Hook<uint32_t>(RB_SwapBuffersHook_Address, hkRB_SwapBuffers);

	// dont take a screenshot in CL_endScene since we do that manually 
	uint32_t patchSize = gameVariant == GameVariant::Stock ? 17 : 28;
	for (size_t i = 0; i < patchSize; i++)
		*(BYTE*)(CL_RunOncePerClientFramePatch_Address + i) = 0x90;

	// CL_Disconnect is called whenever a demo ends
	CL_Disconnect_tramp = Hook<void(*)(std::int32_t)>(CL_Disconnect_Address, hkCL_Disconnect);


	/* Rewinding */

	// prevent Com_Error "Server time went backwards"
	*(BYTE*)RewindPatch1_Address = 0xEB;

	// prevent "WARNING: CG_ReadNextSnapshot: way out of range"
	*(BYTE*)RewindPatch2_Address = 0xEB;

	// prevent Com_Error "cl->snap.serverTime < cl->oldFrameServerTime"
	*(WORD*)RewindPatch3_Address = 0x9090;

	// prevent Com_Error "CL_CGameNeedsServerCommand: a reliable command was cycled out"
	*(BYTE*)CL_CGameNeedsServerCommandPatch_Address = 0xEB;

	FS_Read_tramp = Hook<FS_Read_t>(FS_Read_Address, hkFS_Read);
}