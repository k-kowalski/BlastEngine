#pragma once
#include <fstream>
#include <string>

class File
{
	std::fstream fileStream;
	static constexpr char DELIMITER = '\n';
public:
	File(std::string fileName);
	~File();

	void Write(const void* buffer, int64_t count);
	void Read(void* buffer, int64_t count);

	template <class Ty>
	File& operator<<(const Ty value)
	{
		static_assert(std::is_pod<Ty>::value);
		Write(std::addressof(value), sizeof(Ty));
		return *this;
	};

	template <class Ty>
	File& operator>>(Ty& value)
	{
		static_assert(std::is_pod<Ty>::value);
		Read(std::addressof(value), sizeof(Ty));
		return *this;
	};

	template <>
	File& File::operator<<(const std::string value)
	{
		fileStream << value << DELIMITER;
		return *this;
	}

	template <>
	File& File::operator>>(std::string& value)
	{
		std::getline(fileStream, value, DELIMITER);
		return *this;
	}
};
