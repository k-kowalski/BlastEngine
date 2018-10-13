#include "Texture2D.h"
#include "Graphics.h"
#include "Diagnostics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"


Graphics::Texture2D::Texture2D(std::string fileName)
{
	HRESULT result;
	unsigned char* data = stbi_load(fileName.c_str(), &width, &height, nullptr, STBI_rgb_alpha);

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA subresource;
	ZeroMemory(&subresource, sizeof(subresource));
	subresource.pSysMem = data;
	subresource.SysMemPitch = width * STBI_rgb_alpha;

	ID3D11Device* graphicsDevice = getGraphicsDevice();
	ID3D11Texture2D* textureHandle;
	result = graphicsDevice->CreateTexture2D(&textureDesc, &subresource, &textureHandle);
	if (FAILED(result))
	{
		Diagnostics::message("Failed to create texture.");
		return;
	}

	result = graphicsDevice->CreateShaderResourceView(textureHandle, nullptr, &shaderResourceViewHandle);
	if (FAILED(result))
	{
		Diagnostics::message("Failed to create shader resource view");
		textureHandle->Release();
		return;
	}
	textureHandle->Release();

	stbi_image_free(data);
}


Graphics::Texture2D::~Texture2D()
{
	if (shaderResourceViewHandle != nullptr)
	{
		shaderResourceViewHandle->Release();
	}
}
