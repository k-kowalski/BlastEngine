#include <atlbase.h>
#include <atlwin.h>

class BaseWindow : public CWindowImpl<BaseWindow>
{
private:
	BEGIN_MSG_MAP(DWindow)

	END_MSG_MAP()
protected:
	HWND hWnd;
public:
	BaseWindow(
		int cliWidth,
		int cliHeight,
		int xloc,
		int yloc,
		LPCTSTR szWindowName = 0,
		DWORD dwStyle = 0);
	~BaseWindow();

	HWND getWindowHandle();
	int getClientWidth();
	int getClientHeight();
};
