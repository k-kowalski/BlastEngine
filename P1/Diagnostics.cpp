#include "Diagnostics.h"


namespace Diagnostics
{
	void messageBoxInfoFromHRESULT(HRESULT result)
	{
		_com_error err(result);
		MessageBox(0, err.ErrorMessage(), L"RESULT", MB_OK);
	}

	void messageBoxInfo(std::string message)
	{
		MessageBoxA(0, message.c_str(), "MESSAGE", MB_OK);
	}
}