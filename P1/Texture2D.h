#pragma once
#include <d3d11_4.h>
#include <string>

namespace Graphics
{
	struct Texture2D
	{
		int width, height;
		unsigned char* data;

		ID3D11ShaderResourceView* shaderResourceViewHandle;

		Texture2D(std::string imageFilePath);
		Texture2D(int imageDataLength, unsigned char* imageData);
		~Texture2D();
	};
}
