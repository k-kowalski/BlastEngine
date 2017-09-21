#include "stdafx.h"
#include "BaseWindow.h"

BaseWindow::BaseWindow(
	int cliWidth,
	int cliHeight,
	int xloc,
	int yloc,
	LPCTSTR szWindowName,
	DWORD dwStyle)
{
	RECT bounds = { 0, 0, cliWidth, cliHeight };
	AdjustWindowRect(&bounds, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = Create(0,
		bounds,
		szWindowName,
		dwStyle,
		0,
		0U,
		0);
	
	MoveWindow(xloc, yloc, bounds.right - bounds.left, bounds.bottom - bounds.top, 0);
}

BaseWindow::~BaseWindow()
{
	
}

HWND BaseWindow::getWindowHandle()
{
	return hWnd;
}

int BaseWindow::getClientWidth()
{
	RECT windowRect;
	GetClientRect(&windowRect);

	return windowRect.right - windowRect.left;
}

int BaseWindow::getClientHeight()
{
	RECT windowRect;
	GetClientRect(&windowRect);

	return windowRect.bottom - windowRect.top;
}
