#include "File.h"

File::File(std::string fileName)
{
	fileStream = std::fstream(fileName, std::ios::binary | std::ios::out | std::ios::in);
}

File::~File()
{
	fileStream.close();
}

void File::Write(const void* buffer, int64_t count)
{
	fileStream.write(reinterpret_cast<const char*>(buffer), count);
}

void File::Read(void* buffer, int64_t count)
{
	fileStream.read(reinterpret_cast<char*>(buffer), count);
}
