#pragma once

extern bool Demo_IsPlaying();
extern bool Demo_IsPaused();
extern void Demo_TogglePause();
extern uint32_t Demo_GetCurrentTick();
extern uint32_t Demo_GetStartTick();
extern uint32_t Demo_GetEndTick();
extern std::string Demo_GetName();
extern std::string Demo_GetNameEscaped();

extern void Demo_SkipForward(uint32_t amount);
extern void Demo_SkipBack();
extern void Demo_Rewind();

extern std::uint32_t snapshotIntervalTime;
extern std::string current_dir;
extern uint32_t	fileNum;

extern void Patch_Demo();

struct CustomSnapshot
{
	uint32_t fileOffset = 0;
	uint32_t serverTime = 0;			// -> 0xC5F940
	uint32_t clientConfigNumCoD4X = 0;	// -> 0x9562AC
	uint32_t axisScore = 0;				// -> 0x79DBE8;
	uint32_t alliesScore = 0;			// -> 0x79DBEC;

	int landTime = 0;					// -> 0x7975F8
	char snapshots[12180 * 32];			// -> 0xCC9180 - 32 snapshots
	char entities[244 * 2048];			// -> 0xD65400 - 2048 entities
	char clients[100 * 2048];			// -> 0xDE5EA4 - 2048 clients
	int parseEntitiesNum = 0;			// -> 0xC84F58 - global entity index
	int parseClientsNum = 0;			// -> 0xC84F5C - global client index

	char chat[1320];					// -> 0x74B798
	int serverCommandNum = 0;			// -> 0x7713D8
	int serverCommandSequence1 = 0;		// -> 0x74A91C
	int serverCommandSequence2 = 0;		// -> 0x914E20
};