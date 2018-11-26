#include "Controller.h"
#include "Diagnostics.h"
#include "Graphics.h"
#include "Platform.h"
#include <iostream>
#include <Windowsx.h>
#include <thread>


namespace Controller
{
	int previousMousePositionX = 0;
	int previousMousePositionY = 0;


	// main processor is now processRealtimeInput() but detecting keydowns/ups might be useful eventually
//	bool W_pressed = false;
//	bool A_pressed = false;
//	bool S_pressed = false;
//	bool D_pressed = false;

	void handleDropFiles(WPARAM wParam)
	{
//		// function returns length without null terminator
//		UINT fileNameLength = DragQueryFileA(reinterpret_cast<HDROP>(wParam), 0, nullptr, 0) + 1;
//		char* fileName = new char[fileNameLength];
//		DragQueryFileA(reinterpret_cast<HDROP>(wParam), 0, fileName, fileNameLength);
//		Graphics::loadGeometry( std::string(fileName) );
//
//		delete[] fileName;
	}

	/* scrolling scales object */
	void handleScroll(WPARAM wParam)
	{
		// depending on the direction of the scroll we shall get either -120 or 120 in $scrollDeltaValue
		auto scrollDeltaValue = GET_WHEEL_DELTA_WPARAM(wParam);

		// outward
		if (scrollDeltaValue < 0)
		{
			Graphics::modifyScale(0.9f);
		}
		// inward
		else
		{
			Graphics::modifyScale(1.1f);
		}
	}

	void handleENTERPressed()
	{
		Platform::toggleSystemConsole();
	}

	//	void handleArrowRightPressed()
	//	{
	//	}
	//	
	//	void handleArrowLeftPressed()
	//	{
	//	}
	//	
	//	void handleArrowUpPressed()
	//	{
	//		
	//	}
	//	
	//	void handleArrowDownPressed()
	//	{
	//	}

	void handleMouseMove(WPARAM wParam, LPARAM lParam)
	{
		int mousePositionX = GET_X_LPARAM(lParam);
		int mousePositionY = GET_Y_LPARAM(lParam);

		if (wParam == MK_RBUTTON)
		{
//			printf("x: %d\n", mousePositionX);
//			printf("y: %d\n", mousePositionY);

			//Graphics::updateCameraAngles(previousMousePositionX - mousePositionX, previousMousePositionY - mousePositionY);
			Graphics::updateCameraAngles(mousePositionX - previousMousePositionX, mousePositionY - previousMousePositionY);
		}

		previousMousePositionX = mousePositionX;
		previousMousePositionY = mousePositionY;
	}

	void handleSize(HWND hWnd)
	{
		Graphics::resizeBuffers(hWnd);
	}

	void processRealtimeInput()
	{
		if (GetAsyncKeyState(0x57/* W key */) & 0x8000)
		{
			Graphics::moveCameraForward();
		}
		if (GetAsyncKeyState(0x41/* A key */) & 0x8000)
		{
			Graphics::moveCameraLeft();
		}
		if (GetAsyncKeyState(0x53/* S key */) & 0x8000)
		{
			Graphics::moveCameraBackward();
		}
		if (GetAsyncKeyState(0x44/* D key */) & 0x8000)
		{
			Graphics::moveCameraRight();
		}
		if (GetAsyncKeyState(VK_SPACE) & 0x8000)
		{
			Graphics::moveCameraUpward();
		}
		if (GetAsyncKeyState(0x58/* X key */) & 0x8000)
		{
			Graphics::moveCameraDownward();
		}
	}

	//	void toggleCameraMovementInputHandler()
//	{
//		static bool isRunning;
//
//		isRunning = !isRunning;
//		if (isRunning)
//		{
//
//			cameraMovementInputHandlerThread = new std::thread([]()-> void {
//				while (isRunning)
//				{
//					if (W_pressed)
//					{
//						Graphics::moveCameraForward();
//					}
//					Sleep(10);
//				}
//			});
//			cameraMovementInputHandlerThread->detach();
//		}
//		else
//		{
//			//Diagnostics::message("!!");
//			//delete cameraMovementInputHandlerThread;
//		}
//
//	}

	LRESULT CALLBACK renderWindowMessageProcessor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		{
			// input handled by IMGUI
			return true;
		}

		switch (message)
		{
		case WM_MOUSEMOVE:
			handleMouseMove(wParam, lParam);
			break;
		case WM_DROPFILES:
			handleDropFiles(wParam);
			break;
		case WM_MOUSEWHEEL:;
			handleScroll(wParam);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
//		case WM_KEYUP:
//			switch (wParam)
//			{
//			case 0x57: // W key
//				W_pressed = false;
//				break;
//			case 0x41: // A key
//				A_pressed = false;
//				break;
//			case 0x53: // S key
//				break;
//			case 0x44: // D key
//				break;
//			}
		case WM_SIZE:
			handleSize(hWnd);
			break;
		case WM_KEYDOWN:
			switch (wParam)
			{
//			case 0x57: // W key
//				W_pressed = true;
//				break;
//			case 0x41: // A key
//				A_pressed = true;
//				break;
//			case 0x53: // S key
//				break;
//			case 0x44: // D key
//				break;
			case VK_SPACE:
				break;
			case VK_ESCAPE:
				// exit
				DestroyWindow(hWnd);
				break;
			case VK_RETURN:
				handleENTERPressed();
				break;
			}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}

		return 0;
	}
}
