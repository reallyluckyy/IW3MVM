#include "stdafx.hpp"
#include "Streams.hpp"
#include "Demo.hpp"
#include "Utility.hpp"
#include "Game.hpp"
#include "Dollycam.hpp"
#include "Drawing.hpp"
#include "Freecam.hpp"
#include "Console.hpp"

const std::uint32_t		frameWaitTime = 2;

bool				writeAeCamHeader = false;
RecordingStatus		recordingStatus = RecordingStatus::Disabled;
StreamsOrder		streamsOrder = StreamsOrder::Prepare;
std::uint32_t		maxfps_cap = 1000;

std::uint32_t		queueArrayIndex = 0;
std::vector			<std::string> queueTokens;

std::uint32_t		startShotCount = 0;
std::uint32_t		shotCount = 0;

std::vector			<dvar_s> savedVars;

void Write_CameraData(std::string path, bool finish)
{
	const off_t				sizeOffset = 0x19;
	static dvar_s*			sunDir;
	static dvar_s*			sunCol;
	static std::string		fixedPath;
	static std::uint32_t	filenum = 0;
	std::ofstream file;

	SHCreateDirectoryEx(NULL, path.substr(0, path.find_last_of("\\")).c_str(), NULL);

	if (finish == true)
	{
		char		numFramesW[10];
		FILE*		seeker;

		memset(numFramesW, ' ', 10);
		seeker = fopen(fixedPath.c_str(), "rb+");
		if (!seeker)
		{
			std::printf("Could not finish writing camera data\n");
			return;
			//	do what?
		}
		itoa(shotCount - startShotCount, numFramesW, 10);
		size_t i;
		for (i = 0; numFramesW[i] != '\0'; i++);

		numFramesW[i] = ' ';
		_fseeki64(seeker, sizeOffset, SEEK_SET);
		fwrite(numFramesW, 10, 1, seeker);
		fclose(seeker);
		return;
	}

	if (writeAeCamHeader == false)
	{
		std::ofstream header;

		sunDir = Dvar_FindVar("r_lightTweakSunDirection");
		sunCol = Dvar_FindVar("r_lightTweakSunColor");
		fixedPath = path + std::string(".AECAM");
		header.open(fixedPath.c_str(), std::ios_base::app);
		header << std::fixed << "MVM_HEADER: VERSION 1.1" << "\n";
		header << std::fixed << "0000000000" << "\n";
		header.close();
		writeAeCamHeader = true;
	}
	
	file.open(fixedPath.c_str(), std::ios_base::app);
	file << std::fixed << "3DCAM ";
	file << std::fixed << *player_pos[0] << " ";
	file << std::fixed << *player_pos[1] << " ";
	file << std::fixed << *player_pos[2] << " ";
	file << std::fixed << *player_lookat[0] << " ";
	file << std::fixed << *player_lookat[1] << " ";
	file << std::fixed << *player_lookat[2] << " ";
	file << std::fixed << *player_roll << " ";
	file << std::fixed << (*realFov_divisor * 2.f) << "\n";

	if (mvm_streams_aeExport_sun->current.enabled)
	{
		float dirY = sunDir->current.vector[0];
		float dirX = sunDir->current.vector[1];
		dirX = fmodf(dirX, 360);
		dirY = fmodf(dirY, 360);
		float SunDirectionX2Rad = dirX * (M_PI / 180);
		float SunDirectionY2Rad = dirY * (M_PI / 180);

		float sunX, sunY, sunZ;
		sunX = cos(SunDirectionX2Rad)
			* cos(SunDirectionY2Rad);
		sunY = sin(SunDirectionX2Rad)
			* cos(SunDirectionY2Rad);
		sunZ = sin(SunDirectionY2Rad);
		
		/* not sure if necessary but always normalize for best results */
		float mag = sqrtf(sunX * sunX + sunY * sunY + sunZ * sunZ);
		sunX /= mag;
		sunY /= mag;
		sunZ /= mag;

		float sunR = sunCol->current.color[0];
		float sunG = sunCol->current.color[1];
		float sunB = sunCol->current.color[2];

		file << std::fixed << "3DSUN ";
		file << std::fixed << sunX << " ";
		file << std::fixed << sunY << " ";
		file << std::fixed << sunZ << " ";
		file << std::fixed << sunR << " ";
		file << std::fixed << sunG << " ";
		file << std::fixed << sunB << "\n";
	}
	file.close();
}

void Reset_Recording()
{
	if (!strcmp(mvm_export_format->current.string, "avi\0"))
		AVStream_Finish();

	if (mvm_streams_aeExport->current.enabled)
		Write_CameraData("", true);

	if (queueTokens.empty())
	{
		Console::Log(RED, "queueTokens was empty when resetting recording; this shouldnt happen (%i, %i)", is_recording_streams, is_recording_avidemo);
	}
	else
	{
		for (size_t i = 0; i < savedVars.size(); i++)
		{
			dvar_s* varAddress = Dvar_FindVar(savedVars[i].name);
			if (varAddress == NULL)
				continue;
			memcpy(&varAddress->current, &savedVars[i].current, sizeof(DvarValue));
		}
		savedVars.clear();
		//Cmd_ExecuteSingleCommand(("exec " + queueTokens[0] + "\n").c_str());
		queueTokens.clear();
	}

	recordingStatus = RecordingStatus::Disabled;
	streamsOrder = StreamsOrder::Prepare;
	is_recording_streams = false;
	is_recording_avidemo = false;

	if (!Demo_IsPaused())
		Demo_TogglePause();
	if (*(toggle_console.value) != toggle_console.original)
		*(toggle_console.value) = toggle_console.original;
	
	writeAeCamHeader = false;
	demoDrawMenu = true;

	Console::Log(BLUE, "Saved recording!");
}

static void	parseAndSaveDvars(const char* var)
{
	dvar_s* varAddress;
	dvar_s	varEntry;
	std::string	path = std::string(Dvar_FindVar("fs_homepath")->current.string) + "\\main\\" + var + ".cfg";
	std::ifstream infile(path);

	std::string	dvarName;
	while (infile >> dvarName)
	{
		if (dvarName == "")
			continue;
		for (size_t i = 0; i < savedVars.size(); i++)
		{
			//	Skip if already in the list
			if (strcmp(dvarName.c_str(), savedVars[i].name) == 0)
				break;
		}
		varAddress = Dvar_FindVar(dvarName);
		if (varAddress == NULL)
			continue;

		memcpy(&varEntry, varAddress, sizeof(dvar_s));
		savedVars.push_back(varEntry);
	}	
}

void Streams_Frame()
{
	static bool is_tga = false;
	static int skip_frame = 0;

	//	Skip frames before anything happens
	if (skip_frame >= 0)
	{
		skip_frame -= 1;
		return;
	}
	
	//	Set up streams on first frame
	if (streamsOrder == StreamsOrder::Prepare)
	{
		Console::Log(BLUE, "\nStarting streams recording...");

		if (!queueTokens.empty())
			queueTokens.clear();
		startShotCount = shotCount;

		Dvar_FindVar("cl_avidemo")->current.integer = 0;
		if (is_recording_avidemo)
		{
			Console::Log(RED, "is_recording_avidemo was true while recording streams; this should never happen");
			return;
		}


		queueArrayIndex = 0;
		if (!Demo_IsPaused())
			Demo_TogglePause();

		std::stringstream stream_queue(mvm_streams_passes->current.string);
		std::string queue_current_token;
		while (getline(stream_queue, queue_current_token, ' '))
		{
			if (queue_current_token != "")
			{
				parseAndSaveDvars(queue_current_token.c_str());
				queueTokens.push_back(queue_current_token);
			}
		}
		if (queueTokens.size() == 1)
		{
			Console::Log(BLUE, "\nSingle pass found in mvm_streams_passes. Use mvm_avidemo_fps instead for an increased performance.");
		}
		update_outdir_path(Demo_GetNameEscaped());
		if (!strcmp(mvm_export_format->current.string, "avi\0")) //AVI
		{
			if (!AVStream_Start(queueTokens))
			{
				streamsOrder = StreamsOrder::Prepare;
				return;
			}
			is_tga = false;
		}
		else if (!strcmp(mvm_export_format->current.string, "tga\0"))
		{
			is_tga = true;
		}
		else
		{
			Reset_Recording();
			std::printf("No output format set! (mvm_export_format tga/avi)\n");
		}

		if (*(toggle_console.value) == 1)
		{
			*(toggle_console.value) = 0;
			toggle_console.original = 1;
		}
		demoDrawMenu = false;

		// we are running
		recordingStatus = RecordingStatus::Running;
		streamsOrder = StreamsOrder::Capture;

		Cmd_ExecuteSingleCommand(("exec " + queueTokens[queueArrayIndex] + "\n").c_str());
		*(uint32_t*)cls_realtime_Address += (uint32_t)1;

		// initial delay, ensure the HUD/Console is gone
		skip_frame = mvm_streams_interval->current.integer + 10;
		return;
	}
	//	Execute config on current frame
	else if (streamsOrder == StreamsOrder::Execute)
	{
		Cmd_ExecuteSingleCommand(("exec " + queueTokens[queueArrayIndex] + "\n").c_str());
		skip_frame = mvm_streams_interval->current.integer;
		streamsOrder = StreamsOrder::Capture;
	}
	//	Screenshot and redirect to screenshot() -> @Utility.cpp
	else if (streamsOrder == StreamsOrder::Capture)
	{
		frameData_t data;

		data.streamNum = queueArrayIndex;
		data.name = queueTokens[queueArrayIndex];
		data.frameNum = shotCount - startShotCount;

		if (!is_tga) //AVI
		{
			// Don't write image file, store in libav instead
			data.writeToDisk = false;
		}
		else
		{
			data.path = va("%s\\%s\\%s_%07i.tga",
				standard_output_directory.c_str(),
				queueTokens[queueArrayIndex].c_str(),
				queueTokens[queueArrayIndex].c_str(),
				shotCount);
			data.writeToDisk = true;
		}
		

		bool success = Screenshot(data);
		if (success == false)
			return;

		queueArrayIndex += 1;
		// when the final stream has been captured
		if (queueArrayIndex == queueTokens.size())
		{
			queueArrayIndex = 0;
			if (mvm_streams_aeExport->current.enabled)
			{
				skip_frame = mvm_streams_interval->current.integer;
				streamsOrder = StreamsOrder::Export;
			}
			else
				streamsOrder = StreamsOrder::Advance;
		}
		else
		{
			streamsOrder = StreamsOrder::Execute;
			skip_frame = mvm_streams_interval->current.integer;
		}
		return;
	}
	//	Increment progress by max_rate / frame_rate
	else if (streamsOrder == StreamsOrder::Advance)
	{
		skip_frame = mvm_streams_interval->current.integer;
		shotCount += 1;

		if (recordingStatus == RecordingStatus::Finish)
		{
			recordingStatus = RecordingStatus::Finished;
			return;
		}

		streamsOrder = StreamsOrder::Execute;
		if (currentDollycamState == DollycamState::Disabled && !currentView.isPOV)
			return;
		if (currentDollycamState == DollycamState::Playing&& mvm_dolly_frozen->current.enabled && !currentView.isPOV)
		{
			frozenDollyCurrentFrame += (long)(mvm_dolly_frozen_speed->current.value * timescale->current.value);
			return;
		}
		*(uint32_t*)cls_realtime_Address += 1000 / mvm_streams_fps->current.integer;
		return;
	}
	//	Export data if selected
	else if (streamsOrder == StreamsOrder::Export)
	{
		Write_CameraData(va("%s\\%s",
			standard_output_directory.c_str(),
			Demo_GetNameEscaped().c_str()), false);
		streamsOrder = StreamsOrder::Advance;
		return;
	}
}

void Avidemo_Frame()
{
	static bool is_tga = false;
	const std::string passes = "out";
	static int skip_frame = 0;

	if (skip_frame >= 0)
	{
		skip_frame -= 1;
		return;
	}

	if (streamsOrder == StreamsOrder::Prepare)
	{
		Console::Log(BLUE, "\nStarting avidemo recording...");


		if (!queueTokens.empty())
			queueTokens.clear();
		queueTokens.push_back("out");


		if (is_recording_streams)
		{
			Console::Log(CONSOLE_COLOR::RED, "is_recording_streams was true while recording avidemo; this should never happen");
			return;
		}

		startShotCount = shotCount;

		if (!Demo_IsPaused())
			Demo_TogglePause();

		skip_frame = 2;

		recordingStatus = RecordingStatus::Running;
		streamsOrder = StreamsOrder::Capture;
		if (*(toggle_console.value) == 1)
		{
			*(toggle_console.value) = 0;
			toggle_console.original = 1;
		}
		demoDrawMenu = false;

		update_outdir_path(Demo_GetNameEscaped());

		if (!strcmp(mvm_export_format->current.string, "avi\0")) //AVI
		{
			is_tga = false;
			if (!AVStream_Start(queueTokens))
			{
				streamsOrder = StreamsOrder::Prepare;
				return;
			}
		}
		else if (!strcmp(mvm_export_format->current.string, "tga\0"))
		{
			is_tga = true;
		}
		else
		{
			Reset_Recording();
			std::printf("No output format set! (mvm_export_format tga/avi)\n");
		}

		recordingStatus = RecordingStatus::Running;

		*(uint32_t*)cls_realtime_Address += (uint32_t)1;
		return;
	}
	if (streamsOrder == StreamsOrder::Capture)
	{
		frameData_t data;

		data.frameNum = shotCount - startShotCount;
		data.streamNum = 0;

		if (!is_tga) //AVI
		{
			data.writeToDisk = false;
		}
		else
		{
			data.path = va("%s\\%s\\%07i.tga",
				standard_output_directory.c_str(),
				Demo_GetNameEscaped().c_str(),
				shotCount);
			data.writeToDisk = true;
		}

		bool success = Screenshot(data);
		if (success == false)
			return;
		if (mvm_streams_aeExport->current.enabled)
		{
			if (filenum)
				Write_CameraData(va("%s\\%s_%d", standard_output_directory.c_str(), Demo_GetNameEscaped().c_str(), filenum), false);
			else
				Write_CameraData(va("%s\\%s", standard_output_directory.c_str(), Demo_GetNameEscaped().c_str()), false);
			skip_frame = 1;
			streamsOrder = StreamsOrder::Export;
		}
		else
			streamsOrder = StreamsOrder::Advance;

		return;
	}
	else if (streamsOrder == StreamsOrder::Advance)
	{
		// Fix for stuttery cinematics. Need to investigate what actually causes this; probably async between R_SetViewParmsForScene and this
		if (!currentView.isPOV)
			skip_frame = mvm_streams_interval->current.integer + 1;
		shotCount += 1;

		if (recordingStatus == RecordingStatus::Finish)
		{
			recordingStatus = RecordingStatus::Finished;
			return;
		}

		streamsOrder = StreamsOrder::Capture;
		if (currentDollycamState == DollycamState::Disabled && !currentView.isPOV)
			return;
		if (currentDollycamState == DollycamState::Playing && mvm_dolly_frozen->current.enabled && !currentView.isPOV)
		{
			frozenDollyCurrentFrame += (long)(mvm_dolly_frozen_speed->current.value * timescale->current.value);
			return;
		}
		*(uint32_t*)cls_realtime_Address += 1000 / mvm_avidemo_fps->current.integer;
		return;
	}
	else if (streamsOrder == StreamsOrder::Export)
	{
		
		streamsOrder = StreamsOrder::Advance;
	}
}
