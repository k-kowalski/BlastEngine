#include "stdafx.h"
#include "DWindow.h"

//needed libraries
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib") 
#pragma comment (lib, "D3DCompiler.lib")

//entry point
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	
	DWindow dw(1280, 720, 200, 100, L"My Engine", WS_OVERLAPPEDWINDOW);

	//communication loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;	
}