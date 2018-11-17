#pragma once
#include <comdef.h>
#include <string>

namespace Diagnostics
{
	void messageBoxInfoFromHRESULT(HRESULT result);
	void messageBoxInfo(std::string message);
}