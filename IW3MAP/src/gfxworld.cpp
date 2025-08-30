#include "gfxworld.hpp"

#pragma region

std::vector<std::string> Split(std::string input, std::string delimiter)
{
	std::vector<std::string> strings;
	int pos = input.find(delimiter, 0);

	strings.push_back(input.substr(0, pos));

	while (input.find(delimiter, pos + 1) != input.npos)
	{
		strings.push_back(input.substr(pos + 1, input.find(delimiter, pos + 1) - (pos + 1)));

		pos = input.find(delimiter, pos + 1);
	}

	strings.push_back(input.substr(pos + 1));

	/*for (std::string string : strings)
	{
		std::cout << string << "/";
	}

	std::cout << "\n";*/

	return strings;
}

#ifdef DEBUG
std::string VertToString(codm_Vert vert)
{
	std::string string;

	string = "pos: " + std::to_string(vert.pos[0]) + " " + std::to_string(vert.pos[1]) + " " + std::to_string(vert.pos[2]) + " ";
	string += "normal: " + std::to_string(vert.normal[0]) + " " + std::to_string(vert.normal[1]) + " " + std::to_string(vert.normal[2]) + " ";
	string += "uvs: ";

	for (codm_UvCoord uv : vert.uv)
	{
		string += std::to_string(uv.uv[0]) + " " + std::to_string(uv.uv[1]) + " ";
	}

	string += "colors: ";

	for (unsigned char color : vert.color)
	{
		string += std::to_string((int)color);
	}

	string += "\n";

	return string;
}
#endif

#pragma endregion conversion functions

#pragma region

bool ManhattanDistance(codm_Vert& vert1, codm_Vert& vert2)
{
	if (pow(vert1.pos[0] - vert2.pos[0], 2) > 0.0001)
		return false;
	if (pow(vert1.pos[1] - vert2.pos[1], 2) > 0.0001)
		return false;
	if (pow(vert1.pos[2] - vert2.pos[2], 2) > 0.0001)
		return false;

	if (pow(vert1.normal[0] - vert2.normal[0], 2) > 0.001)
		return false;
	if (pow(vert1.normal[1] - vert2.normal[1], 2) > 0.001)
		return false;
	if (pow(vert1.normal[2] - vert2.normal[2], 2) > 0.001)
		return false;

	return true;
}

bool Overlapping(codm_Face& face1, codm_Face& face2, codm_Face& newFace, short& matCount1, short& matCount2)
{
	short i = 0;

	for (codm_Vert& vert1 : face1.vertices)
	{
		for (codm_Vert& vert2 : face2.vertices)
		{
			if (ManhattanDistance(vert1, vert2))
			{
				newFace.vertices[i] = vert1;

				for (short j = 0; j < matCount2; j++)
				{
					newFace.vertices[i].color[matCount1 + j] = vert2.color[j];
					newFace.vertices[i].uv[matCount1 + j] = vert2.uv[j];
				}

				i++;
				break;
			}
		}
		if (i == 0)
			return false;
	}

	return i == 3;
}

codm_Object SplitInternal(codm_Object& object)
{
	codm_Object newObject;

	newObject.name = object.name + "|second";

	newObject.materialCount = object.materialCount;
	newObject.materials[0] = object.materials[0];
	newObject.materials[0].name += "|second";

	// find overlapping faces

	std::vector<int> doubleFaces;
	codm_Face dummyFace;

	for (int face1 = 0; face1 < object.faces.size(); face1++)
	{
		for (int face2 = face1 + 1; face2 < object.faces.size(); face2++)
		{
			if (Overlapping(object.faces[face1], object.faces[face2], dummyFace, object.materialCount, object.materialCount))
			{
				doubleFaces.push_back(face1);
				break;
			}
		}
	}

	for (int i = doubleFaces.size() - 1; i >= 0; i--)
	{
		newObject.faces.push_back(object.faces[doubleFaces[i]]);
		object.faces.erase(object.faces.begin() + doubleFaces[i]);
	}

	return newObject;
}

bool CanBeMerged(codm_Object& object1, codm_Object& object2)
{
	// should not happen
	/*if (object2.name.find("mixmat;") == 0)
		return false;*/

	for (short i = 0; i < object1.materialCount; i++)
	{
		for (short j = 0; j < object2.materialCount; j++)
		{
			codm_Material& mat1 = object1.materials[i];
			codm_Material& mat2 = object2.materials[j];

			if (mat1.sortKey == mat2.sortKey)
			{
				if (mat1.sortKey == 12 || mat1.sortKey == 10)
					continue;

				if (mat1.name.npos != mat1.name.find("|second") || mat2.name.npos != mat2.name.find("|second"))
				{
					if (mat1.name.find("|second") != mat1.name.npos)
					{
						if (mat1.name.substr(0, mat1.name.find("|second")) == mat2.name)
							continue;
					}
					else if (mat2.name.find("|second") != mat2.name.npos)
					{
						if (mat2.name.substr(0, mat2.name.find("|second")) == mat1.name)
							continue;
					}
				}

				return false;
			}
		}
	}

	return true;
}

codm_Object MergeObjects(codm_Object& object1, codm_Object& object2)
{
	codm_Object newObject;

	if (!(object1.name.find("mixmat;") == 0))
		newObject.name = "mixmat;" + object1.name + ":" + object2.name;
	else
		newObject.name = object1.name + ":" + object2.name;

	// assign new materials

	int j = 0;

	for (short i = 0; i < object1.materialCount; i++)
	{
		newObject.materials[i] = object1.materials[i];
		j++;
	}

	for (short i = 0; i < object2.materialCount; i++)
	{
		newObject.materials[i + j] = object2.materials[i];
	}

	newObject.materialCount = object1.materialCount + object2.materialCount;

	// find overlapping faces

	std::vector<int> removableFaces1;
	int matching[2];

	for (int face1 = 0; face1 < object1.faces.size(); face1++)
	{
		bool broken = false;

		for (int face2 = 0; face2 < object2.faces.size(); face2++)
		{
			codm_Face newFace;

			if (Overlapping(object1.faces[face1], object2.faces[face2], newFace, object1.materialCount, object2.materialCount))
			{
				newObject.faces.push_back(newFace);

				matching[0] = face1;
				matching[1] = face2;

				broken = true;
				break;
			}
		}

		if (broken)
		{
			removableFaces1.push_back(matching[0]);
			object2.faces.erase(object2.faces.begin() + matching[1]);
		}
	}

	std::sort(removableFaces1.begin(), removableFaces1.end());

	for (int i = removableFaces1.size() - 1; i >= 0; i--)
	{
		object1.faces.erase(object1.faces.begin() + removableFaces1[i]);
	}

	return newObject;
}

void RecMerge(int object1Index, int object2Index, std::vector<codm_Object>& objects, int initialSize)
{
	codm_Object newObject = MergeObjects(objects[object1Index], objects[object2Index]);

	if (newObject.faces.size() > 0)
	{
		objects.push_back(newObject);

		int currentSize = objects.size();

		for (int i = 0; i < initialSize; i++)
		{
			if (CanBeMerged(objects[currentSize - 1], objects[i]))
			{
				RecMerge(currentSize - 1, i, objects, initialSize);
			}
		}
	}
}

void Merge(std::vector<codm_Object>& objects)
{
	ProgressBar progressBar;
	// remove internal doubles

	std::vector<codm_Object> newObjects;

	int splitProgress = 0;
	int splitGoal = objects.size();
	progressBar.Start("splitting doubles", &splitProgress, splitGoal);

	for (codm_Object& object : objects)
	{
		progressBar.Update();

		codm_Object newObject = SplitInternal(object);
		if (newObject.faces.size() > 0)
		{
			newObjects.push_back(newObject);
		}
		splitProgress++;
	}

	progressBar.End();

	for (codm_Object& object : newObjects)
	{
		objects.push_back(object);
	}

	// merge objects

	int initialSize = objects.size();

	int pairProgress = 0;
	int pairGoal = (initialSize - 1) * (initialSize / 2);
	progressBar.Start("merging doubles", &pairProgress, pairGoal);

	for (int i = 0; i < initialSize; i++)
	{
		for (int j = i + 1; j < initialSize; j++)
		{
			if (pairProgress % (pairGoal / 50) == 0)
				progressBar.Update();

			if (CanBeMerged(objects[i], objects[j]))
			{
				RecMerge(i, j, objects, initialSize);
			}

			pairProgress++;
		}
	}

	progressBar.End();

	//remove objects

	std::vector<int> removableObjects;

	for (int i = 0; i < objects.size(); i++)
	{
		if (objects[i].faces.size() == 0)
		{
			removableObjects.push_back(i);
		}
	}

	std::sort(removableObjects.begin(), removableObjects.end());

	for (int i = removableObjects.size() - 1; i >= 0; i--)
	{
		objects.erase(objects.begin() + removableObjects[i]);
	}
}

#pragma endregion merge functions

#pragma region

std::vector<codm_Object> LoadMapObjects(GfxWorld& gfxWorld)
{
	ProgressBar progressBar;

	std::vector<codm_Object> objects;
	std::vector<codm_Vert> vertices;
	codm_Vert newVert;
	codm_UvCoord newUv;

	int progress = 0;
	int goal = gfxWorld.vertexCount + gfxWorld.surfaceCount;
	progressBar.Start("loading gfxWorld", &progress, goal);

	for (int i = 0; i < gfxWorld.vertexCount; i++)
	{
		if (i % 400 == 0)
			progressBar.Update();

		newVert.color[0] = gfxWorld.vertexData.vertices[i].color.array[3];

		newVert.pos[0] = gfxWorld.vertexData.vertices[i].xyz[0];
		newVert.pos[1] = gfxWorld.vertexData.vertices[i].xyz[1];
		newVert.pos[2] = gfxWorld.vertexData.vertices[i].xyz[2];

		float* normal = UnpackPackedUnitVec(gfxWorld.vertexData.vertices[i].normal);

		newVert.normal[0] = normal[0];
		newVert.normal[1] = normal[1];
		newVert.normal[2] = normal[2];

		newUv.uv[0] = gfxWorld.vertexData.vertices[i].texCoord[0];
		newUv.uv[1] = (gfxWorld.vertexData.vertices[i].texCoord[1] * -1) + 1;

		newVert.uv[0] = newUv;

		vertices.push_back(newVert);

		progress++;
	}

	for (int i = 0; i < gfxWorld.surfaceCount; i++)
	{
		if (i % 400 == 0)
			progressBar.Update();

		codm_Object* object = nullptr;

		std::string newName = std::string(gfxWorld.surfaces[i].material->info.name);

		for (codm_Object& _object : objects)
		{
			if (_object.name == newName)
			{
				object = &_object;
				break;
			}
		}

		if (object == nullptr)
		{
			codm_Object newObject;
			objects.push_back(newObject);
			object = &objects[objects.size() - 1];
			object->name = newName;
			object->materialCount = 1;

			codm_Material newMaterial;
			newMaterial.name = newName;
			newMaterial.byteFlags = gfxWorld.surfaces[i].material->info.gameFlags;
			newMaterial.sortKey = gfxWorld.surfaces[i].material->info.sortKey;
			newMaterial.textureCount = gfxWorld.surfaces[i].material->textureCount;

			codm_Texture newTexture;

			for (int j = 0; j < gfxWorld.surfaces[i].material->textureCount; j++)
			{
				if (gfxWorld.surfaces[i].material->textureTable[j].semantic == 0xB)
					newTexture.name = "water";//gfxWorld.surfaces[i].material->textureTable[j].u.water->map->image->name;
				else
					newTexture.name = gfxWorld.surfaces[i].material->textureTable[j].u.image->name;
				
				newTexture.type = (unsigned char)gfxWorld.surfaces[i].material->textureTable[j].semantic;
				newMaterial.textures[j] = newTexture;
			}

			object->materials[0] = newMaterial;
		}

		codm_Face newFace;

		for (int j = gfxWorld.surfaces[i].tris.baseIndex; j < gfxWorld.surfaces[i].tris.baseIndex + gfxWorld.surfaces[i].tris.triCount * 3; j += 3)
		{
			newFace.vertices[0] = vertices[gfxWorld.surfaces[i].tris.firstVertex + gfxWorld.indices[j + 0]];
			newFace.vertices[1] = vertices[gfxWorld.surfaces[i].tris.firstVertex + gfxWorld.indices[j + 1]];
			newFace.vertices[2] = vertices[gfxWorld.surfaces[i].tris.firstVertex + gfxWorld.indices[j + 2]];

			object->faces.push_back(newFace);
		}

		progress++;
	}

	progressBar.End();

	return objects;
}

#ifdef DEBUG
std::vector<codm_Object> LoadObj(std::string filepath)
{
	std::ifstream obj;
	obj.open(filepath);
	std::string line;
	
	std::vector<codm_Object> objects;

	std::vector<codm_Vert> vertices;

	std::vector<std::string> strings;
	int start;
	int end;

	std::cout << "loading obj" << std::endl;

	while (std::getline(obj, line))
	{
		if (line.find("O ") == 0)
			continue;

		else if (line.find("v ") == 0)
		{
			codm_Vert newVert;
			start = 2;

			for (int i = 0; i < 3; i++)
			{
				end = line.find(" ", start);
				newVert.pos[i] = stof(line.substr(start, end - start));
				start = end + 1;
			}
			
			end = line.find(" ", start);
			newVert.color[0] = stoi(line.substr(start, end - start));

			vertices.push_back(newVert);
		}
		
		else if (line.find("vt ") == 0)
		{
			codm_UvCoord newUv;
			start = 3;

			for (int i = 0; i < 2; i++)
			{
				end = line.find(" ", start);
				newUv.uv[i] = std::stof(line.substr(start, end - start));
				start = end + 1;
			}

			vertices[vertices.size() - 1].uv[0] = newUv;
		}

		else if (line.find("vn ") == 0)
		{
			start = 3;

			for (int i = 0; i < 3; i++)
			{
				end = line.find(" ", start);
				vertices[vertices.size() - 1].normal[i] = std::stof(line.substr(start, end - start));
				start = end + 1;
			}
		}

		else if (line.find("g ") == 0)
		{
			codm_Object newObject;
			codm_Material newMaterial;
			strings = Split(line.substr(2), ":");

			newObject.name = strings[0];
			newMaterial.name = strings[0];
			newMaterial.sortKey = (short)std::stoi(strings[1]);
			newMaterial.byteFlags = (unsigned char)std::stoi(strings[2]);

			newObject.materials[0] = newMaterial;
			newObject.materialCount = 1;
			objects.push_back(newObject);
		}

		else if (line.find("f ") == 0)
		{
			codm_Face newFace;
			start = 2;

			for (int i = 0; i < 3; i++)
			{
				end = line.find(" ", start);
				newFace.vertices[i] = vertices[std::stoi(line.substr(start, line.find("/", start) - start)) - 1];
				start = end + 1;
			}

			objects[objects.size() - 1].faces.push_back(newFace);
		}
	}

	obj.close();

	std::cout << std::endl;

	return objects;
}

void LoadMmat(std::string filepath, std::vector<codm_Object>& objects)
{
	std::map<std::string, unsigned char> textureTypes;

	textureTypes["TS_COLOR_MAP"]	= (unsigned char)  2;
	textureTypes["TS_NORMAL_MAP"]	= (unsigned char)  5;
	textureTypes["TS_SPECULAR_MAP"] = (unsigned char)  8;
	textureTypes["TS_WATER_MAP"]	= (unsigned char) 11;

	std::ifstream mmat;
	mmat.open(filepath);
	std::string line;

	std::cout << "loading mmat" << std::endl;

	while (std::getline(mmat, line))
	{
		std::vector<std::string> textureStrings = Split(line.substr(0, line.size() - 1), " ");
		std::vector<codm_Texture> newTextures;
		std::string materialName = Split(textureStrings[0], ":")[0];

		for (int i = 1; i < textureStrings.size(); i++)
		{
			codm_Texture newTexture;

			newTexture.name = textureStrings[i].substr(0, textureStrings[i].find(":"));
			newTexture.type = textureTypes[textureStrings[i].substr(textureStrings[i].find(":") + 1)];

			newTextures.push_back(newTexture);
		}

		for (codm_Object& object : objects)
		{
			if (object.materials[0].name == materialName)
			{
				for (short j = 0; j < newTextures.size(); j++)
				{
					object.materials[0].textures[j] = newTextures[j];
				}
				object.materials[0].textureCount = newTextures.size();
			}
		}
	}

	std::cout << std::endl;

	return;
}
#endif

#pragma endregion file functions
