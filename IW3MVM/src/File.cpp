#include "stdafx.hpp"
#include "File.hpp"

std::vector<std::string> File::stringTable;

File::File(std::string path, uint32_t mode)
{
	this->path = path;

	internalFile.open(path, mode);
}

uint32_t File::StoreString(std::string string)
{
	auto result = std::find(stringTable.begin(), stringTable.end(), string);
	if (result != stringTable.end())
		return result - stringTable.begin();

	stringTable.push_back(string);
	return stringTable.size() - 1;
}

void File::WriteStringTable()
{
	Write((uint32_t)stringTable.size());
	for (uint32_t i = 0; i < stringTable.size(); i++)
	{
		this->internalFile.write(stringTable[i].c_str(), strlen(stringTable[i].c_str()));
		Write((char)0x00);
	}
}

void File::Close()
{
	internalFile.close();
}

void File::Jump(uint32_t pos)
{
	internalFile.clear();
	internalFile.seekp(0, pos);
}