#include "Platform.h"
#include "Diagnostics.h"

namespace Platform
{
	ConsoleStream consoleStreamHandle = { nullptr, nullptr, nullptr };

	UINT getClientSpaceWidth(HWND windowHandle)
	{
		RECT windowRect;
		GetClientRect(windowHandle, &windowRect);

		return windowRect.right - windowRect.left;
	}

	UINT getClientSpaceHeight(HWND windowHandle)
	{
		RECT windowRect;
		GetClientRect(windowHandle, &windowRect);

		return windowRect.bottom - windowRect.top;
	}

	void toggleSystemConsole()
	{
		if (consoleStreamHandle.inputStream == nullptr)
		{
			AllocConsole();
			freopen_s(&consoleStreamHandle.inputStream, "CONIN$", "r", stdin);
			freopen_s(&consoleStreamHandle.outputStream, "CONOUT$", "w", stdout);
			freopen_s(&consoleStreamHandle.errorStream, "CONOUT$", "w", stderr);
			printf("SYSTEM: console initialized\n");
		}
		else
		{
			fclose(consoleStreamHandle.inputStream);
			fclose(consoleStreamHandle.outputStream);
			fclose(consoleStreamHandle.errorStream);
			FreeConsole();
			consoleStreamHandle.inputStream = nullptr;
			consoleStreamHandle.outputStream = nullptr;
			consoleStreamHandle.errorStream = nullptr;
		}
	}

	std::string pickFileDialog(HWND dialogParentWindow, char* filter)
	{
		OPENFILENAMEA pickFileStruct;
		char fileNameBuffer[256] = {0};

		UINT structSize = sizeof(pickFileStruct);
		ZeroMemory(&pickFileStruct, structSize);
		pickFileStruct.lStructSize = structSize;
		pickFileStruct.hwndOwner = dialogParentWindow;
		pickFileStruct.lpstrFile = fileNameBuffer;
		pickFileStruct.nMaxFile = sizeof(fileNameBuffer);
		pickFileStruct.lpstrFilter = filter;
		pickFileStruct.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;


		if (GetOpenFileNameA(&pickFileStruct))
		{
			return std::string(fileNameBuffer);
		}
		else
		{
			return std::string("");
		}
	}
}
