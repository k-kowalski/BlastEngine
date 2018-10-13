#pragma once
#include <Windows.h>
#include <cstdio>
#include <array>

namespace Platform
{
	struct ConsoleStream
	{
		FILE* inputStream;
		FILE* outputStream;
		FILE* errorStream;
	};

	extern ConsoleStream consoleStreamHandle;

	UINT getClientSpaceWidth(HWND windowHandle);
	UINT getClientSpaceHeight(HWND windowHandle);
	void toggleSystemConsole();
	std::string pickFileDialog(HWND dialogParentWindow, char* filter);
}
