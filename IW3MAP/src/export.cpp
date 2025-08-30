#include "export.hpp"

std::map<char, std::string> semanticNames{
	{ 0x0, "TS_2D" },
	{ 0x1, "TS_FUNCTION" },
	{ 0x2, "TS_COLOR_MAP" },
	{ 0x3, "TS_UNUSED_1" },
	{ 0x4, "TS_UNUSED_2" },
	{ 0x5, "TS_NORMAL_MAP" },
	{ 0x6, "TS_UNUSED_3" },
	{ 0x7, "TS_UNUSED_4" },
	{ 0x8, "TS_SPECULAR_MAP" },
	{ 0x9, "TS_UNUSED_5" },
	{ 0xA, "TS_UNUSED_6" },
	{ 0xB, "TS_WATER_MAP" },
};

void WriteShort(std::ofstream& ofile, short n)
{
	ofile.write((char*)&n, sizeof(short));
}

void WriteString(std::ofstream& ofile, std::string s)
{
	ofile.write(s.c_str(), s.size());
	char null = 0x00;
	ofile.write(&null, 1);
}

void WriteChar(std::ofstream& ofile, char c)
{
	ofile.write((char*)&c, sizeof(char));
}

void WriteInt(std::ofstream& ofile, int i)
{
	ofile.write((char*)&i, sizeof(int));
}

void WriteFloat(std::ofstream& ofile, float f)
{
	ofile.write((char*)&f, sizeof(float));
}

void WriteXModel(std::ofstream& ofile, XModel* xmodel)
{
	WriteString(ofile, std::string(xmodel->name));

	WriteShort(ofile, xmodel->lodInfo[0].numsurfs);

	for (short i = 0; i < xmodel->lodInfo[0].numsurfs; i++)
	{
		WriteString(ofile, std::string(xmodel->materials[i]->info.name));
		WriteShort(ofile, xmodel->surfaces[i].triCount);

		for (unsigned short j = 0; j < xmodel->surfaces[i].triCount; j++)
		{
			for (short k = 0; k < 3; k++)
			{
				GfxPackedVertex& vert = xmodel->surfaces[i].verts[xmodel->surfaces[i].triIndices[j * 3 + k]];

				WriteFloat(ofile, vert.xyz[0]);
				WriteFloat(ofile, vert.xyz[1]);
				WriteFloat(ofile, vert.xyz[2]);

				float* normal = UnpackPackedUnitVec(vert.normal);

				WriteFloat(ofile, normal[0]);
				WriteFloat(ofile, normal[1]);
				WriteFloat(ofile, normal[2]);

				float* uv = UnpackPackedTexCoords(vert.uv);

				WriteFloat(ofile, uv[0]);
				WriteFloat(ofile, uv[1]);

				WriteChar(ofile, vert.color[3]);
			}
		}
	}
}

void WriteCodm(std::ofstream& ofile, std::vector<codm_Object> objects, std::vector<codm_XModel> xmodels)
{
	ProgressBar progressBar;

	// collect materials

	std::vector<codm_Material> materials;

	for (codm_Object& object : objects)
	{
		for (short i = 0; i < object.materialCount; i++)
		{
			codm_Material& newMaterial = object.materials[i];

			bool broken = false;

			for (codm_Material& material : materials)
			{
				if (newMaterial.name == material.name)
				{
					broken = true;
					break;
				}
			}

			if (!broken)
				materials.push_back(newMaterial);
		}
	}

	for (codm_XModel xmodel : xmodels)
	{
		for (short i = 0; i < xmodel.model->numSurfaces; i++)
		{
			Material mat = *xmodel.model->materials[i];
			codm_Material newMaterial;
			newMaterial.name = std::string(mat.info.name);

			bool broken = false;

			for (codm_Material& material : materials)
			{
				if (newMaterial.name == material.name)
				{
					broken = true;
					break;
				}
			}

			if (!broken)
			{
				newMaterial.byteFlags = mat.info.gameFlags;
				newMaterial.sortKey = mat.info.sortKey;
				newMaterial.textureCount = mat.textureCount;

				for (short j = 0; j < mat.textureCount; j++)
				{
					codm_Texture newTexture;

					if (mat.textureTable[j].semantic == 0xB)
						newTexture.name = mat.textureTable[j].u.water->map->image->name;
					else
						newTexture.name = mat.textureTable[j].u.image->name; 

					newTexture.type = (unsigned char)mat.textureTable[j].semantic;

					newMaterial.textures[j] = newTexture;
				}

				materials.push_back(newMaterial);
			}
		}
	}

	int progress = 0;
	int goal = materials.size() + xmodels.size() + objects.size();
	progressBar.Start("exporting codm", &progress, goal);

	// write materials

	WriteShort(ofile, materials.size());

	for (codm_Material& material : materials)
	{
		progressBar.Update();

		WriteString(ofile, material.name);
		WriteShort(ofile, material.sortKey);
		WriteChar(ofile, material.byteFlags);

		WriteShort(ofile, material.textureCount);
		for (short i = 0; i < material.textureCount; i++)
		{
			WriteString(ofile, material.textures[i].name);
			WriteChar(ofile, material.textures[i].type);
		}

		progress++;
	}

	// write xmodels

	WriteShort(ofile, xmodels.size());

	for (codm_XModel& xmodel : xmodels)
	{
		progressBar.Update();

		WriteXModel(ofile, xmodel.model);

		WriteShort(ofile, xmodel.instances.size());

		for (codm_XModelInstance& instance : xmodel.instances)
		{
			WriteFloat(ofile, instance.pos[0]);
			WriteFloat(ofile, instance.pos[1]);
			WriteFloat(ofile, instance.pos[2]);

			WriteFloat(ofile, instance.rot[0][0]);
			WriteFloat(ofile, instance.rot[0][1]);
			WriteFloat(ofile, instance.rot[0][2]);
			WriteFloat(ofile, instance.rot[1][0]);
			WriteFloat(ofile, instance.rot[1][1]);
			WriteFloat(ofile, instance.rot[1][2]);
			WriteFloat(ofile, instance.rot[2][0]);
			WriteFloat(ofile, instance.rot[2][1]);
			WriteFloat(ofile, instance.rot[2][2]);

			WriteFloat(ofile, instance.scale);
		}

		progress++;
	}

	// write objects

	WriteShort(ofile, objects.size());

	for (short i = 0; i < objects.size(); i++)
	{
		progressBar.Update();

		codm_Object object = objects[i];

		WriteString(ofile, object.name);

		WriteShort(ofile, object.materialCount);

		for (short j = 0; j < object.materialCount; j++)
		{
			WriteString(ofile, object.materials[j].name);
		}

		WriteInt(ofile, object.faces.size());

		for (int j = 0; j < object.faces.size(); j++)
		{
			codm_Face face = object.faces[j];

			for (short k = 0; k < 3; k++)
			{
				codm_Vert vert = face.vertices[k];

				WriteFloat(ofile, vert.pos[0]);
				WriteFloat(ofile, vert.pos[1]);
				WriteFloat(ofile, vert.pos[2]);

				WriteFloat(ofile, vert.normal[0]);
				WriteFloat(ofile, vert.normal[1]);
				WriteFloat(ofile, vert.normal[2]);

				for (short l = 0; l < object.materialCount; l++)
				{
					WriteFloat(ofile, vert.uv[l].uv[0]);
					WriteFloat(ofile, vert.uv[l].uv[1]);

					WriteChar(ofile, vert.color[l]);
				}
			}
		}

		progress++;
	}

	progressBar.End();
}

void WriteObj(std::ofstream& ofile, GfxWorld gfxWorld)
{
	ProgressBar progressBar;

	int progress = 0;
	int goal = gfxWorld.vertexCount + gfxWorld.surfaceCount;
	progressBar.Start("exporting obj", &progress, goal);

	ofile << "O map" << std::endl;

	for (int i = 0; i < gfxWorld.vertexCount; i++)
	{
		if (i % 200 == 0)
			progressBar.Update();

		int alpha = (unsigned char)gfxWorld.vertexData.vertices[i].color.array[3];
		ofile << va("v %g %g %g # %i", gfxWorld.vertexData.vertices[i].xyz[0], gfxWorld.vertexData.vertices[i].xyz[1], gfxWorld.vertexData.vertices[i].xyz[2], alpha) << std::endl;
		ofile << va("vt %g %g", gfxWorld.vertexData.vertices[i].texCoord[0], (gfxWorld.vertexData.vertices[i].texCoord[1] * -1) + 1) << std::endl;
		ofile << va("vn %g %g %g", ((unsigned char)gfxWorld.vertexData.vertices[i].normal.packed / 127.0f - 1) * 1, ((*(((unsigned char*)&gfxWorld.vertexData.vertices[i].normal.packed) + 1)) / 127.0f - 1) * 1, ((*(((unsigned char*)&gfxWorld.vertexData.vertices[i].normal.packed) + 2)) / 127.0f - 1) * 1) << std::endl;

		progress++;
	}

	std::vector<std::pair<Material*, std::vector<std::string>>> materials;

	for (int i = 0; i < gfxWorld.surfaceCount; i++)
	{
		if (i % 200 == 0)
			progressBar.Update();

		for (int j = gfxWorld.surfaces[i].tris.baseIndex; j < gfxWorld.surfaces[i].tris.baseIndex + gfxWorld.surfaces[i].tris.triCount * 3; j += 3)
		{
			std::string data(va("f %i/%i/%i %i/%i/%i %i/%i/%i", gfxWorld.indices[j] + 1 + gfxWorld.surfaces[i].tris.firstVertex, gfxWorld.indices[j] + 1 + gfxWorld.surfaces[i].tris.firstVertex, gfxWorld.indices[j] + 1 + gfxWorld.surfaces[i].tris.firstVertex,
				gfxWorld.indices[j + 1] + 1 + gfxWorld.surfaces[i].tris.firstVertex, gfxWorld.indices[j + 1] + 1 + gfxWorld.surfaces[i].tris.firstVertex, gfxWorld.indices[j + 1] + 1 + gfxWorld.surfaces[i].tris.firstVertex,
				gfxWorld.indices[j + 2] + 1 + gfxWorld.surfaces[i].tris.firstVertex, gfxWorld.indices[j + 2] + 1 + gfxWorld.surfaces[i].tris.firstVertex, gfxWorld.indices[j + 2] + 1 + gfxWorld.surfaces[i].tris.firstVertex));

			bool found = false;
			for (int k = 0; k < materials.size(); k++)
			{
				if (materials[k].first == gfxWorld.surfaces[i].material)
				{
					// add a face to the material 
					materials[k].second.push_back(data);

					found = true;
					break;
				}
			}

			if (!found)
				materials.push_back(std::pair<Material*, std::vector<std::string>>(gfxWorld.surfaces[i].material, std::vector<std::string> {data}));
		}

		progress++;
	}

	for (std::pair<Material*, std::vector<std::string>>& material : materials)
	{
		ofile << va("g %s:%i:%i", material.first->info.name, (DWORD)material.first->info.sortKey, (DWORD)material.first->info.gameFlags) << std::endl;
		for (std::string& surface : material.second)
		{
			ofile << surface.c_str() << std::endl;
		}
	}

	progressBar.End();
}

void WriteMmf(std::ofstream& ofile, GfxWorld gfxWorld)
{
	ProgressBar progressBar;

	int progress = 0;
	int goal = gfxWorld.modelCount;
	progressBar.Start("exporting mmf", &progress, goal);

	for (int i = 0; i < gfxWorld.modelCount; i++)
	{
		if (i % 10 == 0)
			progressBar.Update();

		ofile << va("%s %g %g %g %g %g %g %g %g %g %g %g %g %g", gfxWorld.models[i].model->name, gfxWorld.models[i].placement.origin[0], gfxWorld.models[i].placement.origin[1], gfxWorld.models[i].placement.origin[2],
			gfxWorld.models[i].placement.rotation[0][0], gfxWorld.models[i].placement.rotation[0][1], gfxWorld.models[i].placement.rotation[0][2],
			gfxWorld.models[i].placement.rotation[1][0], gfxWorld.models[i].placement.rotation[1][1], gfxWorld.models[i].placement.rotation[1][2],
			gfxWorld.models[i].placement.rotation[2][0], gfxWorld.models[i].placement.rotation[2][1], gfxWorld.models[i].placement.rotation[2][2],
			gfxWorld.models[i].placement.scale) << std::endl;

		progress++;
	}

	progressBar.End();
}

void WriteMmat(std::ofstream& ofile, GfxWorld gfxWorld)
{
	std::vector<Material*> materials;

	for (int i = 0; i < gfxWorld.surfaceCount; i++)
	{
		for (int j = gfxWorld.surfaces[i].tris.baseIndex; j < gfxWorld.surfaces[i].tris.baseIndex + gfxWorld.surfaces[i].tris.triCount * 3; j += 3)
		{
			bool found = false;
			for (int k = 0; k < materials.size(); k++)
			{
				if (materials[k] == gfxWorld.surfaces[i].material)
				{
					found = true;
					break;
				}
			}

			if (!found)
				materials.push_back(gfxWorld.surfaces[i].material);
		}
	}

	ProgressBar progressBar;

	int progress = 0;
	int goal = materials.size();
	progressBar.Start("exporting mmat", &progress, goal);

	for (Material* material : materials)
	{
		progressBar.Update();

		ofile << va("%s:%i:%i ", material->info.name, material->info.sortKey, material->info.gameFlags);

		for (int i = 0; i < material->textureCount; i++)
		{
			if (material->textureTable[i].semantic == 0xB)
				ofile << va("%s:%s ", material->textureTable[i].u.water->map->image->name, semanticNames[material->textureTable[i].semantic].c_str());
			else
				ofile << va("%s:%s ", material->textureTable[i].u.image->name, semanticNames[material->textureTable[i].semantic].c_str());
		}

		ofile << std::endl;

		progress++;
	}

	progressBar.End();
}