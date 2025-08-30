#pragma once

#include "stdafx.hpp"
#include "game.hpp"

#define MAX_LAYERS 8

struct codm_XModelInstance
{
	float pos[3];
	float rot[3][3];
	float scale;
};

struct codm_XModel
{
	XModel* model;
	std::vector<codm_XModelInstance> instances;
};

struct codm_UvCoord
{
	float uv[2];
};

struct codm_Vert
{
	float pos[3];
	float normal[3];
	codm_UvCoord uv[MAX_LAYERS];
	unsigned char color[MAX_LAYERS];
};

struct codm_Face
{
	codm_Vert vertices[3];
};

struct codm_Texture
{
	unsigned char type;
	std::string name;
};

struct codm_Material
{
	short sortKey;
	unsigned char byteFlags;
	std::string name;
	short textureCount;
	codm_Texture textures[8];
};

struct codm_Object
{
	std::vector<codm_Face> faces;
	short materialCount;
	codm_Material materials[MAX_LAYERS];
	std::string name;
};
