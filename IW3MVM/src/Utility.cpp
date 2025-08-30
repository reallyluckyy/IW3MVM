#include "stdafx.hpp"
#include "Utility.hpp"
#include "Game.hpp"
#include "Freecam.hpp"
#include "Console.hpp"
#include "Demo.hpp"
#include "Dollycam.hpp"
#include "Streams.hpp"
#include <d3d9helper.h>
#include <sys/stat.h>

uint32_t	filenum;

std::string Update_FilePath(std::string path)
{
	static uint32_t filen = 1;
	std::string fixed;

	fixed = path;
	while (Dir_Exists(fixed.c_str()))
	{
		fixed = va("%s_%d", path.c_str(), filen);
		filen += 1;
	}
	filen = 1;
	return (fixed);
}

void	update_outdir_path(std::string demoName)
{
	standard_output_directory = std::string(Dvar_FindVar("fs_homepath")->current.string) + "\\" + std::string(Dvar_FindVar("fs_game")->current.string) + "\\movies";
	if (mvm_output_directory->current.string[0] != '\0')
	{
		if (!strcmp(mvm_output_directory->current.string, "default\0"))
		{
			mvm_output_directory->current.string = "\0";
		}
		if (!Dir_Exists(mvm_output_directory->current.string))
		{
			//	Prevent reckless writes
			Console::Log(RED, "[mvm_output_directory]: Output directory does not exist! Please create one.");
			mvm_output_directory->current.string = "\0";
		}
		else
		{
			std::string path = mvm_output_directory->current.string;
			standard_output_directory = path;
		}
	}
	if (is_recording_streams || !strcmp(mvm_export_format->current.string, "tga\0"))
	{
		//	standard is cod4/mod/movies/demoname_n or customdir/demoname_n
		//	in case of avidemo, it would be cod4/mod/movies
		standard_output_directory = standard_output_directory + "\\" + demoName;//va("%s\\%s", standard_output_directory, demoName.c_str());

		std::string fixed;

		fixed = standard_output_directory;
		while (Dir_Exists(fixed.c_str()))
		{
			fixed = va("%s_%lu", standard_output_directory.c_str(), fileNum);
			fileNum += 1;
		}
		standard_output_directory = fixed;
	}

	//	create directory
	SHCreateDirectoryEx(NULL, standard_output_directory.c_str(), NULL);
}

void UnprotectModule(const char* moduleName)
{
	HMODULE module = GetModuleHandle(moduleName);

	PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD)module + header->e_lfanew);

	SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
	DWORD oldProtect;
	VirtualProtect((LPVOID)module, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}

uint32_t FindPattern(std::string pattern, uint32_t dataOffset, bool abortOnFailure, const char* moduleName)
{
	std::vector<std::string> bytes;
	
	std::stringstream ss(pattern);
	std::string current;
	while (std::getline(ss, current, ' '))
		bytes.push_back(current);

	uintptr_t baseAddress = (uintptr_t)GetModuleHandle(moduleName);
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)baseAddress;
	PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((DWORD)dosHeader + dosHeader->e_lfanew);
	uint32_t matchCount = 0;
	uint32_t offset = 0;
	while(offset < ntHeaders->OptionalHeader.SizeOfCode)
	{
		if (matchCount == bytes.size())
		{
			if (dataOffset == 0)
				return baseAddress + ntHeaders->OptionalHeader.BaseOfCode + (offset - matchCount);
			else
				return *(uint32_t*)(baseAddress + ntHeaders->OptionalHeader.BaseOfCode + (offset - matchCount) + dataOffset);
		}

		if (bytes.at(matchCount).compare("??") == 0)
		{
			matchCount++;
			offset++;
			continue;
		}

		if (std::stoul(bytes.at(matchCount).c_str(), nullptr, 16) ==
			*(BYTE*)(baseAddress + ntHeaders->OptionalHeader.BaseOfCode + offset))
		{
			matchCount++; 
			offset++;
		}
		else
		{
			if (!matchCount)
				offset++;
			matchCount = 0;
		}
	}

	if (abortOnFailure)
	{
		Console::Log(RED, "Failed to initialize IW3MVM!");
		Console::Log(YELLOW, "Current game version: %s", Dvar_FindVar("version")->current.string);
		Console::Log(YELLOW, "Please update to COD4 1.7!");

		Sleep(8000);
		ExitProcess(0);
	}

	return 0;
}

R_TakeScreenshot_t R_TakeScreenshot;

#define _BYTE char
#define BYTEn(x, n)   (*((_BYTE*)&(x)+n))
#define BYTE1(x)   BYTEn(x,  1)

void* memalloc(size_t size)
{
	void *result = NULL;

	while (result == NULL)
		result = std::malloc(size);
	return result;
}

std::string cstr_to_string(char *str, int size)
{
	std::string s = "";

	for (int i = 0; i < size; i++) {
		s = s + str[i];
	}
	return (s);
}

void Screenshot_cod4(std::string path)
{
	// recursively create path folders if not present
	SHCreateDirectoryEx(NULL, path.substr(0, path.find_last_of("\\")).c_str(), NULL);

	// build a byte buffer to hold TGA image data
	int size = 3 * (refdef->windowSize[1] * refdef->windowSize[0] + 6);
	char* memory = (char*)memalloc(size);

	// prepare header info
	*memory = 0;
	*(memory + 1) = 0;
	*(memory + 2) = 0;
	*(memory + 3) = 0;
	*(memory + 8) = 0;
	memory[0xD] = BYTE1(refdef->windowSize[0]);
	memory[2] = 2;
	memory[0xC] = refdef->windowSize[0];
	memory[0xE] = refdef->windowSize[1];
	memory[0xF] = BYTE1(refdef->windowSize[1]);
	memory[0x10] = 24;
	memory[0x11] = 32;

	// get actual image data
	R_TakeScreenshot(refdef->windowSize[0], refdef->windowSize[1], (char*)(memory + 0x12));

	// write the TGA bytes to file
	std::ofstream file;
	file.open(path.c_str(), std::ios::binary | std::ios::out);
	file.write((char*)(memory), size);
	file.close();

	free(memory);
}

std::vector <void *> gmAviFiles;

bool AVStream_Finish()
{
	if (gmAviFiles.empty())
		return (false);

	for (size_t i = 0; i < gmAviFiles.size(); i++) {
		gmav_finish(gmAviFiles[i]);
	}
	gmAviFiles.clear();
	return (true);
}

static dvar_s* zDepthEnabled;

bool AVStream_Start(std::vector <std::string> queue)
{
	std::string		OutPath;
	void*			avEntry = NULL;

	if (!gmAviFiles.empty())
	{
		std::printf("Detected previous render as failed\n");
		gmAviFiles.clear();
	}
	zDepthEnabled = Dvar_FindVar("r_showFloatZDebug");

	if (is_recording_streams)
	{
		for (uint32_t i = 0; i < queue.size(); i++) {
			std::string		OutFile;

			//OutFile = standard_output_directory + "\\" + queue[i] + ".avi";
			OutFile = va("%s\\%s", standard_output_directory.c_str(), va("%s.avi", queue[i].c_str()));

			avEntry = gmav_open(
				OutFile.c_str(),
				refdef->windowSize[0],
				refdef->windowSize[1],
				mvm_streams_fps->current.integer);
			if (avEntry == NULL)
			{
				std::printf("avEntry is NULL\n");
				Reset_Recording();
			}
			gmAviFiles.push_back(avEntry);
			avEntry = NULL;
		}
	}
	else if (is_recording_avidemo)
	{
		OutPath = standard_output_directory + "\\" + Demo_GetNameEscaped() + ".avi";
		filenum = 0;

		for (int j = 1; File_Exists(OutPath); j++)
		{
			OutPath = va("%s\\%s_%d.avi",
				standard_output_directory.c_str(),
				Demo_GetNameEscaped().c_str(),
				j);
			filenum = j;
		}

		avEntry = gmav_open(
			OutPath.c_str(),
			refdef->windowSize[0],
			refdef->windowSize[1],
			mvm_avidemo_fps->current.integer);
		if (avEntry == NULL)
			return (false);
		gmAviFiles.push_back(avEntry);
	}
	return (true);
}

float	saturate(const float x)
{
	if (x > 1.f)
		return 1.0f;
	if (x < 0.f)
		return 0.f;
	return (x);
}

bool AddFrameToAVStream(frameData_t frameData)
{
	
	uint8_t* buf24pp = (uint8_t*)malloc(refdef->windowSize[0] * refdef->windowSize[1] * 3);
	if (buf24pp == NULL)
		return (false);
	for (int y = 0; y < refdef->windowSize[1]; y++) {
		for (int x = 0; x < refdef->windowSize[0]; x++) {
			uint32_t	pixel_pos32 = (y * refdef->windowSize[0] * 4) + x * 4;

			uint32_t	bottom_row = refdef->windowSize[1] - y - 1;
			//uint32_t	pixel_pos24 = ((refdef->windowSize[0] - (y + 1)) * refdef->windowSize[0] * 3) + x * 3;
			uint32_t	pixel_pos24 = (bottom_row * refdef->windowSize[0] * 3) + x * 3;

			if (zDepthEnabled->current.integer == 1 && mvm_streams_depthFormat->current.integer != 1)
			{
				// Make depth greyscale
				uint8_t	floatZ = *(uint8_t*)(frameData.image_buffer + pixel_pos32);

				float	b = *(uint8_t*)(frameData.image_buffer + pixel_pos32);
				float	g = *(uint8_t*)(frameData.image_buffer + pixel_pos32 + 1);
				float	r = *(uint8_t*)(frameData.image_buffer + pixel_pos32 + 2);

				g *= 4.f;
				b *= 16.f;

				float	zDepth;
				if (r < 255.f)
					zDepth = r;
				else if (g < 1020.f)
					zDepth = g;
				else
					zDepth = b;
				zDepth += 4;
				
				float value = 1.f - (zDepth / 4084.f);
				
				if (mvm_streams_depthFormat->current.integer == 2)
				{
					b = fabs(value * 6.0f - 3.0f) - 1.0f;
					g = 2.0f - fabs(value * 6.0f - 2.0f);
					r = 2.0f - fabs(value * 6.0f - 4.0f);

					r = saturate(r) * 255.f;
					g = saturate(g) * 255.f;
					b = saturate(b) * 255.f;
				}
				else
				{
					r = (1.0f - value) * 255.f;
					g = (1.0f - value) * 255.f;
					b = (1.0f - value) * 255.f;
				}
				
				*(uint8_t*)(buf24pp + pixel_pos24    ) = (int)r;
				*(uint8_t*)(buf24pp + pixel_pos24 + 1) = (int)g;
				*(uint8_t*)(buf24pp + pixel_pos24 + 2) = (int)b;
			}
			else
				memcpy((uint8_t*)(buf24pp + pixel_pos24), (uint8_t*)(frameData.image_buffer + pixel_pos32), 3);
		}
	}
	if (gmav_add(gmAviFiles[frameData.streamNum], buf24pp) == false)
	{
		free(buf24pp);
		return (false);
	}
	free(buf24pp);
	/* YUV CONVERSION, useful for later */
	/*if (mLibav[s_idx].CodecContext->pix_fmt == AV_PIX_FMT_YUV420P)
	{
		frame->data[0] = picture_buf;
		frame->data[1] = frame->data[0] + size;
		frame->data[2] = frame->data[1] + size / 4;
		frame->linesize[0] = mLibav[s_idx].CodecContext->width;
		frame->linesize[1] = mLibav[s_idx].CodecContext->width / 2;
		frame->linesize[2] = mLibav[s_idx].CodecContext->width / 2;

		// RGB -> YUV conversion
		for (int y = 0; y < mLibav[s_idx].CodecContext->height; y++) {
			for (int x = 0; x < mLibav[s_idx].CodecContext->width; x++) {
				unsigned char b = frameData.image_buffer[(y * mLibav[s_idx].CodecContext->width + x) * 4 + 0];
				unsigned char g = frameData.image_buffer[(y * mLibav[s_idx].CodecContext->width + x) * 4 + 1];
				unsigned char r = frameData.image_buffer[(y * mLibav[s_idx].CodecContext->width + x) * 4 + 2];

				unsigned char Y = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
				frame->data[0][y * frame->linesize[0] + x] = Y;
				if (x % 2 == 0)
				{
					unsigned char V = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
					unsigned char U = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;

					frame->data[1][y / 2 * frame->linesize[1] + x / 2] = U;
					frame->data[2][y / 2 * frame->linesize[2] + x / 2] = V;
				}
			}
		}
	}*/

	return (true);
}

bool Screenshot(frameData_t frameData)
{
	static IDirect3DDevice9* device = NULL;
	D3DRASTER_STATUS	RasterStatus;
	IDirect3DSurface9*	RenderTarget = NULL;
	IDirect3DSurface9*	RenderSurface = NULL;
	D3DLOCKED_RECT		RenderSurfaceData;
	D3DSURFACE_DESC		desc;
	HRESULT				result;

	if (device == NULL)
		device = *(IDirect3DDevice9**)0xCC9A408;

	// Check raster status
	result = device->GetRasterStatus(0, &RasterStatus);
	if (result != D3D_OK || RasterStatus.InVBlank)
		return (false);

	result = device->GetRenderTarget(0, &RenderTarget);
	if (FAILED(result))
	{
		RenderTarget->Release();
		return (false);
	}
	result = RenderTarget->GetDesc(&desc);

	if (desc.Width != refdef->windowSize[0] || desc.Height != refdef->windowSize[1])
	{
		RenderTarget->Release();
		return (false);
	}
	result = device->CreateRenderTarget(
		desc.Width,
		desc.Height,
		D3DFORMAT::D3DFMT_A8R8G8B8,
		D3DMULTISAMPLE_TYPE::D3DMULTISAMPLE_NONE,
		0,
		true,
		&RenderSurface,
		NULL);

	if (FAILED(result))
	{
		RenderSurface->Release();
		return (false);
	}

	result = device->StretchRect(
		RenderTarget,
		NULL,
		RenderSurface,
		NULL,
		D3DTEXTUREFILTERTYPE::D3DTEXF_NONE);

	if (FAILED(result))
	{
		RenderSurface->Release();
		RenderTarget->Release();
		return (false);
	}

	result = RenderSurface->LockRect(
		&RenderSurfaceData,
		NULL,
		D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK);

	if (FAILED(result))
	{
		RenderSurface->Release();
		RenderTarget->Release();
		return (false);
	}

	

	/* if we wish to compress */
	if (frameData.writeToDisk == false)
	{
		frameData.image_buffer = (uint8_t*)(RenderSurfaceData.pBits);
		bool success = AddFrameToAVStream(frameData);
		RenderSurface->UnlockRect();
		RenderSurface->Release();
		RenderTarget->Release();
		return (success);
	}
	else 
	{
		size_t header_size = 18;
		size_t buffer_size = 4 * desc.Width * desc.Height;

		SHCreateDirectoryEx(NULL, frameData.path.substr(0, frameData.path.find_last_of("\\")).c_str(), NULL);
		char* header = (char*)malloc(header_size);
		if (header == NULL)
		{
			RenderSurface->Release();
			RenderTarget->Release();
			return (false);
		}

		// prepare header info
		*header = 0;
		*(header + 1) = 0;
		*(header + 2) = 0;
		*(header + 3) = 0;
		*(header + 8) = 0;
		header[0xD] = BYTE1(refdef->windowSize[0]);
		header[2] = 2;
		header[0xC] = refdef->windowSize[0];
		header[0xE] = refdef->windowSize[1];
		header[0xF] = BYTE1(refdef->windowSize[1]);
		header[0x10] = 32;
		header[0x11] = 32;

		// write the TGA bytes to file
		std::ofstream file;
		file.open(frameData.path.c_str(), std::ios::binary | std::ios::out);
		file.write((char*)(header), header_size);
		file.write((char*)(RenderSurfaceData.pBits), buffer_size);
		file.close();
		free(header);
		RenderSurface->UnlockRect();
		RenderSurface->Release();
		RenderTarget->Release();
	}
	
	return (true);
}

void MakeCall(BYTE* address, uint32_t hook, std::size_t len)
{
	DWORD old;
	VirtualProtect(address, len, PAGE_EXECUTE_READWRITE, &old);

	*address = 0xE8;
	*(DWORD*)((DWORD)address + 1) = (DWORD)hook - (DWORD)address - 5;

	VirtualProtect(address, len, old, &old);

	if (len > 5)
	{
		VirtualProtect(address + 5, len - 5, PAGE_EXECUTE_READWRITE, &old);
		std::memset(address + 5, 0x90, len - 5);
		VirtualProtect(address + 5, len - 5, old, &old);
	}
}

uint32_t SL_ConvertToString_Address;
char* SL_ConvertToString(short num)
{
	if (num)
		return (char*)((*(DWORD*)SL_ConvertToString_Address) + 0xC * num + 0x4);

	return 0;
}

int Dir_Exists(const char* const path)
{
	struct stat info;

	int statRC = stat(path, &info);
	if (statRC != 0)
	{
		if (errno == ENOENT) { return 0; } // something along the path does not exist
		if (errno == ENOTDIR) { return 0; } // something in path prefix is not a dir
		return -1;
	}

	return (info.st_mode & S_IFDIR) ? 1 : 0;
}



bool	File_Exists(std::string path)
{
	std::fstream f(path);
	bool good = f.good();
	f.close();
	return good;
}

bool VerifyGameVersion(std::string supportedVersion)
{
	std::string currentGameVersion(Dvar_FindVar("version")->current.string);

	if (currentGameVersion.compare(supportedVersion) != 0)
	{
		return false;
	}
	
	return true;
}

uint32_t Con_IsVisible_Address;
bool Con_IsVisible()
{
	return *(int*)Con_IsVisible_Address;
}

bool IsKeyPressed(int key)
{
	//	Requires reinit for active window?
	if (GetAsyncKeyState(key) && currentDollycamState != DollycamState::Playing)
		return true;
	return false;
}

void ToggleGreenscreen()
{
	dvar_s* r_clear = Dvar_FindVar("r_clear");

	if (r_clear->current.integer == 0)
	{
		// turn on greenscreen
		r_clear->current.integer = 3;
		Cmd_ExecuteSingleCommand("r_clearcolor 0 1 0 1\n");
		Cmd_ExecuteSingleCommand("r_clearcolor2 0 1 0 1\n");
		Cmd_ExecuteSingleCommand("r_skippvs 1\n");
		Cmd_ExecuteSingleCommand("fx_enable 0\n");
		Cmd_ExecuteSingleCommand("fx_marks 0\n");
		Cmd_ExecuteSingleCommand("cg_draw2D 0\n");

		if (currentView.isPOV)
		{
			Cmd_ExecuteSingleCommand("r_zfar 0.001\n");
			Cmd_ExecuteSingleCommand("r_znear 10000\n");
		}
	}
	else
	{
		r_clear->current.integer = 0;
		Cmd_ExecuteSingleCommand("r_skippvs 0\n");
		Cmd_ExecuteSingleCommand("fx_enable 1\n");
		Cmd_ExecuteSingleCommand("fx_marks 1\n");

		if (Dvar_FindVar("r_znear")->current.value > 9000)
		{
			Cmd_ExecuteSingleCommand("r_zfar 0\n");
			Cmd_ExecuteSingleCommand("r_znear 4\n");
		}
	}
}


#define VA_BUFFER_COUNT		4
#define VA_BUFFER_SIZE		4096

static char g_vaBuffer[VA_BUFFER_COUNT][VA_BUFFER_SIZE];
static int g_vaNextBufferIndex = 0;

char* va(const char *fmt, ...)
{
	va_list ap;
	char* dest = &g_vaBuffer[g_vaNextBufferIndex][0];
	g_vaNextBufferIndex = (g_vaNextBufferIndex + 1) % VA_BUFFER_COUNT;
	va_start(ap, fmt);
	vsprintf_s(dest, VA_BUFFER_SIZE, fmt, ap);
	va_end(ap);
	return dest;
}