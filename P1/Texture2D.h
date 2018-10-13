#pragma once
#include <d3d11_4.h>
#include <string>

namespace Graphics
{
	struct Texture2D
	{
		int width, height;

		ID3D11ShaderResourceView* shaderResourceViewHandle;

		Texture2D(std::string imageFilePath);
		~Texture2D();
	};
}
