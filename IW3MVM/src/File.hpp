#pragma once
#include "Utility.hpp"
#include "IW3D.hpp"

class File
{
private:
	std::fstream internalFile;
	static std::vector<std::string> stringTable;

public:

	std::string path;

	File(std::string path, uint32_t mode);

	void Close();

	void Jump(uint32_t pos);

	uint32_t StoreString(std::string string);

	void WriteStringTable();

	template<typename T>
	void Write(T value)
	{
		if (this->internalFile.is_open())
		{
			this->internalFile.write(reinterpret_cast<char*>(&value), sizeof(T));
		}
	}

	template <>
	void Write<const char*>(const char* string)
	{
		if (this->internalFile.is_open())
		{
			Write(StoreString(string));
		}
	}

	template <>
	void Write<char*>(char* string)
	{
		Write((const char*)string);
	}
};