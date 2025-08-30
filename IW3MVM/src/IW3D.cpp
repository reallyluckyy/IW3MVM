#include "stdafx.hpp"
#include "IW3D.hpp"
#include "Utility.hpp"
#include "Console.hpp"
#include "Drawing.hpp"
#include "Game.hpp"
#include "Demo.hpp"
#include "Freecam.hpp"
#include "File.hpp"

#include <chrono>
#include <thread>
#include <algorithm>

float lastFovValue;

centity_t* cg_entities;
cg_t* cg;
WeaponDef** weaponDefs;
clientinfo_t* clientinfo;

// part of cg->snap->playerState i think
int* localPlayerHealth;

// this is actually some floating-point value that is somehow related
// to the viewmodel placement (recoil something maybe?)
int* viewMuzzle;


// Wrapper around AimTarget_GetTagPos 
uint32_t AimTarget_GetTagPos_Address;
int AimTarget_GetTagPos(uint16_t tagName, centity_t* entity, float* pos)
{
	int returnValue = 0;

	__asm
	{
		push	pos
		movzx	esi, tagName
		mov		ecx, entity
		movzx	eax, byte ptr[ecx + 4]
		mov		edi, AimTarget_GetTagPos_Address
		call	edi
		add		esp, 4
		mov		returnValue, eax
	}

	return returnValue;
}

uint32_t RegisterTag_Address;
uint16_t RegisterTag(const char* tagName, int entityType, int tagLen)
{
	__asm
	{
		push tagLen
		push entityType
		push tagName
		mov eax, RegisterTag_Address
		call eax
		add esp, 0xC
	}
}

typedef bool(*CG_DObjGetWorldtagMatrix_t)(centity_t* entity, int pose, float *origin); // tagName on ecx, tagMat on edi
CG_DObjGetWorldtagMatrix_t CG_DObjGetWorldtagMatrix_Internal;

bool CG_DObjGetWorldTagMatrix(int tagName, float(*tagMat)[3], centity_t* entity, int pose, float* origin)
{
	__asm mov ecx, tagName
	__asm mov edi, tagMat
	return CG_DObjGetWorldtagMatrix_Internal(entity, pose, origin);
}


DWORD AimTarget_GetTagPos_tramp;
DWORD AimTarget_GetTagPos_Continue;
DWORD AimTarget_GetTagPos_End;
int AimTarget_GetTagPos_currentPose;

centity_t* viewmodelEntity;
centity_t* currentEntity;
std::string currentBoneName;
vec3_t currentBoneRotation[3];
vec3_t currentBoneOrigin;
bool isCustomCall = false; // whether we're calling AimTarget_GetTagPos or whether the game is calling it

uint32_t poseArray_Address;
uint32_t iw3dUnknown1_Address;
uint32_t iw3dUnknown2_Address;

void AimTarget_GetTagPos_Hook()
{
	// TODO: also do this when we're just displaying the bones with a potential iw3d_debug dvar
	if (iw3d_fps->current.integer && isCustomCall)
	{
		if (currentEntity == viewmodelEntity)
		{
			// the current entity is the viewmodel entity
			uint32_t v0 = *(uint32_t*)iw3dUnknown1_Address;

			if (!((*(uint32_t*)iw3dUnknown2_Address) & 2))
				v0 = *(uint32_t*)(iw3dUnknown1_Address + 0x8);

			uint32_t v1 = 0x11 * v0;
			uint32_t* viewmodelPose = (uint32_t*)(4 * v1 + poseArray_Address);
			uint32_t viewmodelDObj = ((uint32_t*)poseArray_Address)[17 * v0];

			if (viewmodelDObj)
			{
				AimTarget_GetTagPos_currentPose = *viewmodelPose;
				CG_DObjGetWorldTagMatrix(RegisterTag(currentBoneName.c_str(), 1, currentBoneName.size() + 1), currentBoneRotation, currentEntity, AimTarget_GetTagPos_currentPose, currentBoneOrigin);
			}
			else
			{
				currentBoneOrigin[0] = 0;
				currentBoneOrigin[1] = 0;
				currentBoneOrigin[2] = 0;

				for (int i = 0; i < 3; i++)
					for (int j = 0; j < 3; j++)
						currentBoneRotation[i][j] = 0;
			}
		}
		else
		{
			CG_DObjGetWorldTagMatrix(RegisterTag(currentBoneName.c_str(), 1, currentBoneName.size() + 1), currentBoneRotation, currentEntity, AimTarget_GetTagPos_currentPose, currentBoneOrigin);
		}

		isCustomCall = false;
		AimTarget_GetTagPos_Continue = AimTarget_GetTagPos_End;
	}
	else
	{
		AimTarget_GetTagPos_Continue = AimTarget_GetTagPos_tramp;
	}
}

void __declspec(naked) hkAimTarget_GetTagPos()
{
	__asm pushad
	__asm mov AimTarget_GetTagPos_currentPose, edi

	AimTarget_GetTagPos_Hook();

	__asm popad
	__asm jmp AimTarget_GetTagPos_Continue
}

bool __forceinline GetPlayerBone(int clientnum, std::string bone)
{
	vec3_t pos;
	isCustomCall = true;
	currentBoneName = bone;
	currentEntity = &cg_entities[clientnum];
	return AimTarget_GetTagPos(RegisterTag(bone.c_str(), 1, bone.size() + 1), &cg_entities[clientnum], pos);
}

bool __forceinline GetViewmodelBone(std::string bone)
{
	vec3_t pos;
	isCustomCall = true;
	currentBoneName = bone;
	currentEntity = (centity_t*)viewmodelEntity;
	return AimTarget_GetTagPos(RegisterTag(bone.c_str(), 1, bone.size() + 1), &cg_entities[cg->clientnum], pos);
}

uint64_t iw3dFrameCount = 0;

void IW3D_Frame()
{
#ifdef IW3D_EXPORT
	iw3dFrameCount++;

	Screenshot_cod4(va("%s\\IW3D\\%s\\%i.tga", Dvar_FindVar("fs_homepath")->current.string, Demo_GetNameEscaped().c_str(), frameCount));

	std::string demoName = Demo_GetNameEscaped();

	File file(va("IW3D\\%s.iw3d", demoName.c_str()), std::ios_base::app | std::ios::binary);

	/* Write camera data */

	file.Write(lastFovValue);
	file.Write(currentView.position[0]);
	file.Write(currentView.position[1]);
	file.Write(currentView.position[2]);
	file.Write(currentView.rotation[0]);
	file.Write(currentView.rotation[1]);
	file.Write(currentView.rotation[2]);


	/* Write POV data */

	file.Write(currentView.isPOV);

	if (currentView.isPOV)
	{
		int weaponID = cg_entities[cg->clientnum].weapon;
		WeaponDef* weapon = weaponDefs[weaponID];
		
		file.Write(clientinfo[cg->clientnum].name);

		bool isAlive = localPlayerHealth > 0;

		file.Write(isAlive);
		file.Write(weaponID != 0);

		if (weaponID)
		{
			file.Write(weapon->handXModel[0].name);
			file.Write(weapon->gunXModel[0]->name);
			file.Write(weapon->szInternalName);

			bool isScoped = lastFovValue < (cg_fov->current.value * Dvar_FindVar("cg_fovscale")->current.value);

			file.Write(isScoped);
			file.Write(*viewMuzzle != 0);

			// write viewmodel hand bones
			file.Write(weapon->handXModel->numBones);
			for (int i = 0; i < weapon->handXModel->numBones; i++)
			{
				MessageBox(NULL, va("%X", weapon->handXModel->surfaces[0].vertInfo.vertsBlend), "", 1);

				std::string bone (SL_ConvertToString(weapon->handXModel->boneNames[i]));
				bool result = GetViewmodelBone(bone);

				file.Write(bone.c_str());
				if (result && (currentBoneOrigin[0] != 0 || currentBoneOrigin[1] != 0 || currentBoneOrigin[2] != 0))
				{
					// most likely worked

					file.Write(currentBoneOrigin[0]);
					file.Write(currentBoneOrigin[1]);
					file.Write(currentBoneOrigin[2]);

					for (int i = 0; i < 3; i++)
						for (int j = 0; j < 3; j++)
							file.Write(currentBoneRotation[i][j]);

				}
				else
				{
					file.Write(0.0f);
					file.Write(0.0f);
					file.Write(0.0f);

					for (int j = 0; j < 3; j++)
						for (int k = 0; k < 3; k++)
							file.Write(0.0f);
				}
			}

			// write viewmodel gun bones
			file.Write(weapon->gunXModel[0]->numBones);
			for (int i = 0; i < weapon->gunXModel[0]->numBones; i++)
			{
				std::string bone (SL_ConvertToString(weapon->gunXModel[0]->boneNames[i]));
				bool result = GetViewmodelBone(bone);

				file.Write(bone.c_str());
				if (result && (currentBoneOrigin[0] != 0 || currentBoneOrigin[1] != 0 || currentBoneOrigin[2] != 0))
				{
					// most likely worked

					file.Write(currentBoneOrigin[0]);
					file.Write(currentBoneOrigin[1]);
					file.Write(currentBoneOrigin[2]);

					for (int i = 0; i < 3; i++)
						for (int j = 0; j < 3; j++)
							file.Write(currentBoneRotation[i][j]);
				}
				else
				{
					file.Write(0.0f);
					file.Write(0.0f);
					file.Write(0.0f);

					for (int j = 0; j < 3; j++)
						for (int k = 0; k < 3; k++)
							file.Write(0.0f);
				}
			}
		}
	}


	/* Write player data */

	// get playerCount (only valid players)

	uint8_t playerCount = 0;
	for (int playerID = 0; playerID < 18; playerID++)
	{
		if (!(playerID == cg->clientnum && currentView.isPOV) && cg_entities[playerID].currentValid)
		{
			playerCount++;
		}
	}

	file.Write(playerCount);
	for (int playerID = 0; playerID < 18; playerID++)
	{
		if ((playerID == cg->clientnum && currentView.isPOV) || !cg_entities[playerID].currentValid)
		{
			// dont try to export our own local player if recording in first-person view
			// in that case we already have the POV info
			// also dont export players that are not valid like wtf
			continue;
		}
		else
		{
			int weaponID = cg_entities[playerID].weapon;
			WeaponDef* weapon = weaponDefs[weaponID];

			bool isAlive = (playerID == cg->clientnum) ? (localPlayerHealth > 0) : cg_entities[playerID].isAlive;

			file.Write(clientinfo[playerID].name);
			file.Write(clientinfo[playerID].body);
			file.Write(clientinfo[playerID].head);
			file.Write(isAlive);

			XModel* bodyModel = (XModel*)DB_FindXAssetHeader(0x3, va("%s", clientinfo[playerID].body));
			XModel* headModel = (XModel*)DB_FindXAssetHeader(0x3, va("%s", clientinfo[playerID].head));

			// write body-model bones
			file.Write(bodyModel->numBones);
			for (int i = 0; i < bodyModel->numBones; i++)
			{
				std::string bone (SL_ConvertToString(bodyModel->boneNames[i]));
				bool result = GetPlayerBone(playerID, bone);

				file.Write(bone.c_str());
				if (result && (currentBoneOrigin[0] != 0 || currentBoneOrigin[1] != 0 || currentBoneOrigin[2] != 0))
				{
					// most likely worked
					file.Write(currentBoneOrigin[0]);
					file.Write(currentBoneOrigin[1]);
					file.Write(currentBoneOrigin[2]);

					for (int j = 0; j < 3; j++)
						for (int k = 0; k < 3; k++)
							file.Write(currentBoneRotation[j][k]);

				}
				else
				{
					file.Write(0.0f);
					file.Write(0.0f);
					file.Write(0.0f);

					for (int j = 0; j < 3; j++)
						for (int k = 0; k < 3; k++)
							file.Write(0.0f);
				}
			}

			// write head-model bones
			file.Write(headModel->numBones);
			for (int i = 0; i < headModel->numBones; i++)
			{
				std::string bone(SL_ConvertToString(headModel->boneNames[i]));
				bool result = GetPlayerBone(playerID, bone);

				file.Write(bone.c_str());
				if (result && (currentBoneOrigin[0] != 0 || currentBoneOrigin[1] != 0 || currentBoneOrigin[2] != 0))
				{
					// most likely worked

					file.Write(currentBoneOrigin[0]);
					file.Write(currentBoneOrigin[1]);
					file.Write(currentBoneOrigin[2]);

					for (int j = 0; j < 3; j++)
						for (int k = 0; k < 3; k++)
							file.Write(currentBoneRotation[j][k]);

				}
				else
				{
					file.Write(0.0f);
					file.Write(0.0f);
					file.Write(0.0f);

					for (int j = 0; j < 3; j++)
						for (int k = 0; k < 3; k++)
							file.Write(0.0f);
				}
			}


			file.Write(weaponID != 0);
			if (weaponID)
			{
				file.Write(weapon->worldModel[0]->name);
				file.Write(weapon->szInternalName);

				file.Write(weapon->worldModel[0]->numBones);
				for (int i = 0; i < weapon->worldModel[0]->numBones; i++)
				{
					std::string bone(SL_ConvertToString(weapon->worldModel[0]->boneNames[i]));
					bool result = GetPlayerBone(playerID, bone);
					file.Write(bone.c_str());

					if (result && (currentBoneOrigin[0] != 0 || currentBoneOrigin[1] != 0 || currentBoneOrigin[2] != 0))
					{

						file.Write(currentBoneOrigin[0]);
						file.Write(currentBoneOrigin[1]);
						file.Write(currentBoneOrigin[2]);

						for (int j = 0; j < 3; j++)
							for (int k = 0; k < 3; k++)
								file.Write(currentBoneRotation[j][k]);
					}
					else
					{
						file.Write(0.0f);
						file.Write(0.0f);
						file.Write(0.0f);

						for (int j = 0; j < 3; j++)
							for (int k = 0; k < 3; k++)
								file.Write(0.0f);
					}
				}
			}
		}
	}


	/* Write entity data */

	for (int entityID = 64; entityID < 64 + 12; entityID++)
	{
		file.Write(va("entity_%i", entityID));
		file.Write(clientinfo[cg_entities[entityID].parentEntity].body);
		file.Write(clientinfo[cg_entities[entityID].parentEntity].head);

		XModel* bodyModel = (XModel*)DB_FindXAssetHeader(0x3, va("%s", clientinfo[cg_entities[entityID].parentEntity].body));
		XModel* headModel = (XModel*)DB_FindXAssetHeader(0x3, va("%s", clientinfo[cg_entities[entityID].parentEntity].head));

		// write body-model bones
		file.Write(bodyModel->numBones);
		for (int i = 0; i < bodyModel->numBones; i++)
		{
			std::string bone(SL_ConvertToString(bodyModel->boneNames[i]));
			bool result = GetPlayerBone(entityID, bone);

			file.Write(bone.c_str());
			if (result && (currentBoneOrigin[0] != 0 || currentBoneOrigin[1] != 0 || currentBoneOrigin[2] != 0))
			{
				// most likely worked

				file.Write(currentBoneOrigin[0]);
				file.Write(currentBoneOrigin[1]);
				file.Write(currentBoneOrigin[2]);

				for (int j = 0; j < 3; j++)
					for (int k = 0; k < 3; k++)
						file.Write(currentBoneRotation[j][k]);

			}
			else
			{
				file.Write(0.0f);
				file.Write(0.0f);
				file.Write(0.0f);

				for (int j = 0; j < 3; j++)
					for (int k = 0; k < 3; k++)
						file.Write(0.0f);
			}
		}

		// write head-model bones
		file.Write(headModel->numBones);
		for (int i = 0; i < headModel->numBones; i++)
		{
			std::string bone(SL_ConvertToString(headModel->boneNames[i]));
			bool result = GetPlayerBone(entityID, bone);

			file.Write(bone.c_str());
			if (result && (currentBoneOrigin[0] != 0 || currentBoneOrigin[1] != 0 || currentBoneOrigin[2] != 0))
			{
				// most likely worked

				file.Write(currentBoneOrigin[0]);
				file.Write(currentBoneOrigin[1]);
				file.Write(currentBoneOrigin[2]);

				for (int j = 0; j < 3; j++)
					for (int k = 0; k < 3; k++)
						file.Write(currentBoneRotation[j][k]);

			}
			else
			{
				file.Write(0.0f);
				file.Write(0.0f);
				file.Write(0.0f);

				for (int j = 0; j < 3; j++)
					for (int k = 0; k < 3; k++)
						file.Write(0.0f);
			}
		}
	}

	file.Close();
	
#endif
}

void GetColor(int playerID, vec4_t& color)
{
	switch (playerID)
	{
		case 0:
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
			break;
		case 1:
			color[0] = 0.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case 2:
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case 3:
			color[0] = 0.0f;
			color[1] = 0.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case 4:
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
			break;
		case 5:
			color[0] = 0.0f;
			color[1] = 1.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
			break;
		case 6:
			color[0] = 0.5f;
			color[1] = 1.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
			break;
		case 7:
			color[0] = 0.0f;
			color[1] = 0.5f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case 8:
			color[0] = 0.5f;
			color[1] = 0.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case 9:
			color[0] = 0.5f;
			color[1] = 0.0f;
			color[2] = 0.5f;
			color[3] = 1.0f;
			break;
		case 10:
			color[0] = 0.5f;
			color[1] = 0.5f;
			color[2] = 0.5f;
			color[3] = 1.0f;
			break;
		case 11:
			color[0] = 0.0f;
			color[1] = 1.0f;
			color[2] = 0.5f;
			color[3] = 1.0f;
			break;
		case 12:
			color[0] = 0.0f;
			color[1] = 0.5f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case 13:
			color[0] = 1.0f;
			color[1] = 0.5f;
			color[2] = 0.0f;
			color[3] = 1.0f;
			break;
		case 14:
			color[0] = 0.5f;
			color[1] = 1.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
			break;
		case 15:
			color[0] = 0.5f;
			color[1] = 0.5f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case 16:
			color[0] = 1.0f;
			color[1] = 0.5f;
			color[2] = 0.5f;
			color[3] = 1.0f;
			break;
		case 17:
			color[0] = 0.5f;
			color[1] = 1.0f;
			color[2] = 0.5f;
			color[3] = 1.0f;
			break;
	}
}

void DrawIW3DUI()
{
#ifdef IW3D_EXPORT
	Font* normalFont = CL_RegisterFont("fonts/bigFont", 1);
	Font* objectiveFont = CL_RegisterFont("fonts/objectiveFont", 1);
	Material* white = Material_RegisterHandle("white", 7);

	UI_DrawString(va("^1RECORDING IW3D DATA (%i fps)", iw3d_fps->current.integer), 5, 22, 0.5f, 0.5f, objectiveFont, vec4_t{ 1.0f, 1.0f, 1.0f, 1.0f });

	for (int playerID = 0; playerID < 18; playerID++)
	{
		if ((playerID == cg->clientnum && currentView.isPOV) || !cg_entities[playerID].currentValid || strlen(clientinfo[playerID].name) <= 1)
			continue;

		if (!cg_entities[playerID].isAlive)
			continue;

		vec3_t world{cg_entities[playerID].lerpOrigin[0], cg_entities[playerID].lerpOrigin[1], cg_entities[playerID].lerpOrigin[2] + 70};

		vec2_t position;
		if (WorldToScreen(world, position))
		{
			float distance = VectorDistance(world, currentView.position);
			float size = 10.0f / distance * (1000.0f);

			vec4_t color;
			GetColor(playerID, color);

			UI_DrawRectangle(position[0], position[1], size / 1.25f, size / 1.25f, white, color);

			UI_DrawString(std::string(va("%s", clientinfo[playerID].name)).substr(0, 16).c_str(), 10, 50 + playerID * 30, 1.25f, 1.25f, normalFont, color);
		}
	}

	for (int entityID = 64; entityID < 64 + 12; entityID++)
	{
		if (!cg_entities[entityID].currentValid || cg_entities[cg_entities[entityID].parentEntity].isAlive)
			continue;

		if ((cg_entities[entityID].eFlags & 524288) == 0)
			continue;

		vec3_t world{ cg_entities[entityID].lerpOrigin[0], cg_entities[entityID].lerpOrigin[1], cg_entities[entityID].lerpOrigin[2] + 70 };

		vec2_t position;
		if (WorldToScreen(world, position))
		{
			float distance = VectorDistance(world, currentView.position);
			float size = 10.0f / distance * (1000.0f);

			vec4_t color;
			GetColor(cg_entities[entityID].parentEntity, color);

			UI_DrawRectangle(position[0], position[1], size / 1.25f, size / 1.25f, white, color);

			UI_DrawString(va("entity_%i", entityID), 10, 50 + cg_entities[entityID].parentEntity * 30, 1.25f, 1.25f, normalFont, color);
		}
	}
#endif
}

void Patch_IW3D()
{
#ifdef IW3D_EXPORT

	// get addresses for IW3D-related hooks and so on
	cg_entities = (centity_t*)FindPattern("81 C1 ?? ?? ?? ?? 57 51", 2);
	cg = (cg_t*)FindPattern("68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B C3", 1);
	weaponDefs = (WeaponDef**)FindPattern("8B 14 9D ?? ?? ?? ?? C6 81", 3);
	clientinfo = (clientinfo_t*)FindPattern("81 C7 ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 C4 08", 2);
	localPlayerHealth = (int*)(FindPattern("83 3D ?? ?? ?? ?? ?? 74 43 D9 05", 2) + 0xC + 0x148);
	viewMuzzle = (int*)FindPattern("D9 15 ?? ?? ?? ?? 68 ?? ?? ?? ?? D9 15 ?? ?? ?? ?? 89 1D", 2);
	AimTarget_GetTagPos_Address = FindPattern("51 8D 04 C0 C1 E0 07");
	RegisterTag_Address = FindPattern("83 EC 10 53 8B 5C 24 20 55 56 8B 74 24 20");
	CG_DObjGetWorldtagMatrix_Internal = (CG_DObjGetWorldtagMatrix_t)FindPattern("83 EC 34 53 8B 5C 24 40 55 8B 6C 24 48");
	AimTarget_GetTagPos_End = AimTarget_GetTagPos_Address + 0x67;
	if (*(BYTE*)AimTarget_GetTagPos_End != 0xB8)
	{
		MessageBoxA(0, "An error occured! Please report this to luckyy.", "ERROR", 1);
		ExitProcess(0);
	}
	viewmodelEntity = (centity_t*)FindPattern("68 ?? ?? ?? ?? BF ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 0C 85 C0", 1);
	poseArray_Address = FindPattern("81 C2 ?? ?? ?? ?? 57 52", 2);
	iw3dUnknown1_Address = FindPattern("8B 1D ?? ?? ?? ?? 75 06", 2);
	iw3dUnknown2_Address = FindPattern("F6 05 ?? ?? ?? ?? ?? 8B 1D", 2);

	CreateDirectory("IW3D", NULL);

	// hook AimTarget_GetTagPos to modify it to return position + rotation for any given bone
	AimTarget_GetTagPos_tramp = Hook<uint32_t>(/*FindPattern("8B 44 24 0C 50 51 8B CE E8 ?? ?? ?? ?? 83 C4 08 85 C0 75 25")*/0x4024DE, hkAimTarget_GetTagPos);

	// patch "return 0" into AimTarget_GetTagPos failing branch
	uint32_t AimTarget_GetTagPosFailed_Address = FindPattern("85 F6 74 0F 8B 15 ?? ?? ?? ?? 8D 0C 76 8D 44 8A 04 EB 02");
	*(BYTE*)AimTarget_GetTagPosFailed_Address = 0xB8;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 1) = 0x00;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 2) = 0x00;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 3) = 0x00;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 4) = 0x00;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 5) = 0x5F;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 6) = 0x59;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 7) = 0xC3;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 8) = 0x90;
	*(BYTE*)(AimTarget_GetTagPosFailed_Address + 9) = 0x90;

#endif
}
