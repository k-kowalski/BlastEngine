#pragma once
#include <Windows.h>
#include "imgui/imgui.h"

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace Controller
{

	LRESULT CALLBACK renderWindowMessageProcessor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void handleDropFiles(WPARAM wParam);
	void handleScroll(WPARAM wParam);
	void handleENTERPressed();
	void handleMouseMove(WPARAM w_param, LPARAM l_param);
	void handleSize(HWND hWnd);
	void processRealtimeInput();
}
