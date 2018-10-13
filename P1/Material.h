#pragma once
#include "Texture2D.h"
#include <vector>
#include <memory>

namespace Graphics
{
	struct Material
	{
		std::vector<std::unique_ptr<Texture2D>> textures;
		ID3D11VertexShader* vertexShader;
		ID3D11PixelShader* pixelShader;
		ID3D11InputLayout* vertexInputLayout;
		ID3D11SamplerState* samplerState;

		Material(std::vector<std::unique_ptr<Texture2D>>& textures, std::string shaderSourcePath);
		~Material();

		std::vector<ID3D11ShaderResourceView*> getHandlesForShaderResources();
		static std::vector<D3D11_INPUT_ELEMENT_DESC> createInputLayoutDescFromVertexShaderSignature(ID3DBlob* vertexShaderBlob);
	};
}
