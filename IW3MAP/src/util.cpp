#include "util.hpp"

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

char* SL_ConvertToString(short num)
{
	static char* scr_const = (char*)0x14E8A04;

	if (num)
		return (char*)((*(DWORD*)0x14E8A04) + 0xC * num + 0x4);

	return 0;
}

float* UnpackPackedUnitVec(PackedUnitVec vec)
{
	float* unpacked = new float[3];

	unpacked[0] = ((*(((unsigned char*)&vec.packed) + 3)) / 127.0f - 1);
	unpacked[1] = ((*(((unsigned char*)&vec.packed) + 2)) / 127.0f - 1);
	unpacked[2] = ((*(((unsigned char*)&vec.packed) + 1)) / 127.0f - 1);

	return unpacked;
}

float half2float(const uint16_t s) {
	uint32_t t1;
	uint32_t t2;
	uint32_t t3;

	t1 = s & 0x7fff;						// Non-sign bits
	t2 = s & 0x8000;						// Sign bit
	t3 = s & 0x7c00;						// Exponent

	t1 <<= 13;                              // Align mantissa on MSB
	t2 <<= 16;                              // Shift sign bit into position

	t1 += 0x38000000;                       // Adjust bias

	t1 = (t3 == 0 ? 0 : t1);                // Denormals-as-zero

	t1 |= t2;                               // Re-insert sign bit

	return reinterpret_cast<float&>(t1);
};

float* UnpackPackedTexCoords(PackedTexCoords tex)
{
	float* unpacked = new float[2];

	unpacked[0] = half2float(tex.packed[1]);
	unpacked[1] = half2float(tex.packed[0]) * -1 + 1;

	return unpacked;
}

void ProgressBar::Start(std::string newTask, int* newProgress, int newGoal)
{
	task = newTask;
	progress = newProgress;
	goal = newGoal;

	std::cout << task << std::endl;
	start = GetEpochTime();

	Update();
}

void ProgressBar::Update()
{
	std::cout << "\r";

	char buffer[64];
	buffer[0] = '[';

	float percentage = ((float)*progress / (float)(goal - 1) * 100.0f);

	int roundedPercentage = (int)(percentage / 2 + 0.5);

	for (int i = 0; i < 50; i++)
	{
		if (i < roundedPercentage - 1)
			buffer[i + 1] = '=';
		else if (i == roundedPercentage - 1)
			buffer[i + 1] = '>';
		else
			buffer[i + 1] = ' ';
	}

	buffer[51] = ']';
	buffer[52] = ' ';
	buffer[53] = '\0';

	char buffer2[8];

	sprintf_s(buffer2, "%6.2f%%", percentage);

	std::cout << buffer << buffer2;
}

void ProgressBar::End()
{
	*progress = goal - 1;
	Update();
	std::chrono::milliseconds end = GetEpochTime();
	char buffer[256];
	sprintf_s(buffer, " took %.2f seconds", (double)((end - start).count()) / 1000);
	std::cout << std::endl << task << buffer << std::endl << std::endl;
}

std::chrono::milliseconds ProgressBar::GetEpochTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

bool Demo_IsPaused()
{
	return (*(BYTE*)0x46C94B) == 0x90;
}

void Demo_TogglePause()
{
	DWORD old;
	VirtualProtect((LPVOID)0x46C94B, 0x10, PAGE_EXECUTE_READWRITE, &old);

	if (!Demo_IsPaused())
	{
		for (int i = 0; i < 6; i++)
			*(BYTE*)(0x46C94B + i) = 0x90;
	}
	else
	{
		*(BYTE*)(0x46C94B + 0) = 0x01;
		*(BYTE*)(0x46C94B + 1) = 0x35;
		*(BYTE*)(0x46C94B + 2) = 0x98;
		*(BYTE*)(0x46C94B + 3) = 0x6E;
		*(BYTE*)(0x46C94B + 4) = 0x95;
		*(BYTE*)(0x46C94B + 5) = 0x00;
	}
}