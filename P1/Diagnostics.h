#pragma once
#include <comdef.h>
#include <string>

namespace Diagnostics
{
	void messageFromHRESULT(HRESULT result);
	void message(std::string message);
}