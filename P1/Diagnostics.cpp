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
		if (&message == nullptr || message.c_str() == nullptr)
		{
			MessageBoxA(0, "!!! message is nullptr", "MESSAGE", MB_OK);
		}
		else
		{
			MessageBoxA(0, message.c_str(), "MESSAGE", MB_OK);
		}
	}
}