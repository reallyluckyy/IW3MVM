#include "stdafx.hpp"
#include "Utility.hpp"
#include "Misc.hpp"
#include "Console.hpp"
#include "Game.hpp"

void Patch_Misc()
{
	/* fullscreen depthmap patches */
	uint32_t RB_ShowFloatZDebugPatch_Address = FindPattern("DC C9 D9 C9 D9 5C 24 08 DA 0D");
	*(WORD*)(RB_ShowFloatZDebugPatch_Address - 4) = 0xADA0;
	*(WORD*)(RB_ShowFloatZDebugPatch_Address + 0x4F) = 0xB020;

	/* connection interrupted patches */
	uint32_t CG_DrawConnectionInterrupted_Address = FindPattern("83 EC 28 80 3D");
	*(BYTE*)CG_DrawConnectionInterrupted_Address = 0xC3;
	*(BYTE*)(CG_DrawConnectionInterrupted_Address + 1) = 0x90;
	*(BYTE*)(CG_DrawConnectionInterrupted_Address + 2) = 0x90;
}

void ExtendLODDistance() 
{
	// extend LOD distance
	dvar_s* r_lodBiasRigid = Dvar_FindVar("r_lodBiasRigid");
	r_lodBiasRigid->current.value = -40000;
	r_lodBiasRigid->latched.value = -40000;

	dvar_s* r_lodBiasSkinned = Dvar_FindVar("r_lodBiasSkinned");
	r_lodBiasSkinned->current.value = -40000;
	r_lodBiasSkinned->latched.value = -40000;
}