#include "stdafx.hpp"
#include "Fog.hpp"
#include "Console.hpp"
#include "Utility.hpp"
#include "Demo.hpp"
#include "Game.hpp"

float fogDefaultStart = 0;
float fogDefaultEnd = 0;

uint32_t R_SetSunConstants_tramp;
void __declspec(naked) hkR_SetSunConstants() noexcept
{
	static int* input;

	__asm pushad
	__asm mov input, eax

	if (Demo_IsPlaying())
	{
		if (mvm_fog_custom->current.enabled)
		{
			Dvar_FindVar("r_fog")->current.enabled = true;

			*(float*)(input + 0x294) = 1.0f;
			*(float*)(input + 0x298) = -1.0f / mvm_fog_end->current.value;
			*(float*)(input + 0x29C) = mvm_fog_start->current.value * 0.00083f;

			/* COLORS */

			*(float*)(input + 0x2A0) = ((float)mvm_fog_color->current.color[0] / 255.0f);
			*(float*)(input + 0x2A4) = ((float)mvm_fog_color->current.color[1] / 255.0f);
			*(float*)(input + 0x2A8) = ((float)mvm_fog_color->current.color[2] / 255.0f);
			*(float*)(input + 0x2AC) = ((float)mvm_fog_color->current.color[3] / 255.0f);

			*(float*)0xCEE2B14 = 1.0f;
			*(float*)0xCEE2B18 = -1.0f / mvm_fog_end->current.value;
			*(float*)0xCEE2B1C = mvm_fog_start->current.value * 0.00083f;

			/* COLORS */

			*(float*)0xCEE2B20 = ((float)mvm_fog_color->current.color[0] / 255.0f);
			*(float*)0xCEE2B24 = ((float)mvm_fog_color->current.color[1] / 255.0f);
			*(float*)0xCEE2B28 = ((float)mvm_fog_color->current.color[2] / 255.0f);
			*(float*)0xCEE2B2C = ((float)mvm_fog_color->current.color[3] / 255.0f);

			*(float*)0xCEFC9D4 = 1.0f;
			*(float*)0xCEFC9D8 = -1.0f / mvm_fog_end->current.value;
			*(float*)0xCEFC9DC = mvm_fog_start->current.value * 0.00083f;

			/* COLORS */

			*(float*)0xCEFC9E0 = ((float)mvm_fog_color->current.color[0] / 255.0f);
			*(float*)0xCEFC9E4 = ((float)mvm_fog_color->current.color[1] / 255.0f);
			*(float*)0xCEFC9E8 = ((float)mvm_fog_color->current.color[2] / 255.0f);
			*(float*)0xCEFC9EC = ((float)mvm_fog_color->current.color[3] / 255.0f);

			/* SET DISTANCE */

			if (!fogDefaultStart)
				fogDefaultStart = *(float*)0xCC9F310;

			if (!fogDefaultEnd)
				fogDefaultEnd = *(float*)0xCC9F314;

			*(float*)0xCC9F310 = mvm_fog_start->current.value;
			*(float*)0xCC9F314 = 1.0f / mvm_fog_end->current.value;
		}
		else 
		{
			*(float*)0xCC9F310 = fogDefaultStart;
			*(float*)0xCC9F314 = fogDefaultEnd;
		}
	}

	__asm popad
	__asm jmp R_SetSunConstants_tramp
}

void Patch_Fog()
{
	if (VerifyGameVersion(IW3MVM_SUPPORTED_VERSION))
		R_SetSunConstants_tramp = Hook<uint32_t>(0x5F9700, hkR_SetSunConstants);
	else
		Console::Log(YELLOW, "Disabling fog customization (only available on COD4 1.7)");
}