#include "Material.h"
#include <d3dcompiler.h>
#include <codecvt>
#include "Diagnostics.h"
#include "Graphics.h"

Graphics::Material::Material(std::vector<std::unique_ptr<Texture2D>>& textures, std::string shaderSourcePath)
{
	this->textures = std::move(textures);

	HRESULT result;
	ID3DBlob* errorBlob;

	// create shaders
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wideStringConverter;
	ID3DBlob* vertexShaderBlob;
	result = D3DCompileFromFile(wideStringConverter.from_bytes(shaderSourcePath).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vertexShaderMain", "vs_5_0", 0, 0, &vertexShaderBlob, &errorBlob);
	if (FAILED(result))
	{
		if (errorBlob != nullptr)
		{
			Diagnostics::message("Compilation of vertex shader failed. See error in next message.");
			Diagnostics::message(static_cast<char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
			return;
		}
	}

	ID3DBlob* pixelShaderBlob;
	result = D3DCompileFromFile(wideStringConverter.from_bytes(shaderSourcePath).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "pixelShaderMain", "ps_5_0", 0, 0, &pixelShaderBlob, &errorBlob);
	if (FAILED(result))
	{
		if (errorBlob != nullptr)
		{
			Diagnostics::message("Compilation of pixel shader failed. See error in next message.");
			Diagnostics::message(static_cast<char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
			return;
		}
	}

	ID3D11Device* graphicsDevice = getGraphicsDevice();

	result = graphicsDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader);
	if (FAILED(result))
	{
		Diagnostics::message("Creation of vertex shader - failed.");
		return;
	}

	result = graphicsDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader);
	if (FAILED(result))
	{
		Diagnostics::message("Creation of pixel shader - failed.");
		return;
	}

	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc = createInputLayoutDescFromVertexShaderSignature(vertexShaderBlob);
	result = graphicsDevice->CreateInputLayout(inputLayoutDesc.data(), static_cast<UINT>(inputLayoutDesc.size()), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &vertexInputLayout);
	if (FAILED(result))
	{
		Diagnostics::message("Creation of input layout - failed.");
		return;
	}

	D3D11_SAMPLER_DESC textureSamplerDesc;
	ZeroMemory(&textureSamplerDesc, sizeof(textureSamplerDesc));
	textureSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	textureSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	textureSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	textureSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	textureSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	textureSamplerDesc.MinLOD = 0;
	textureSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	result = graphicsDevice->CreateSamplerState(&textureSamplerDesc, &samplerState);
	if (FAILED(result))
	{
		Diagnostics::message("Failed to create texture sampler state.");
		return;
	}
}


Graphics::Material::~Material()
{
	if (pixelShader != nullptr)
	{
		pixelShader->Release();
	}
	
	if (vertexShader != nullptr)
	{
		vertexShader->Release();
	}
	
	if (vertexInputLayout != nullptr)
	{
		vertexInputLayout->Release();
	}

	if (samplerState != nullptr)
	{
		samplerState->Release();
	}
}

std::vector<ID3D11ShaderResourceView*> Graphics::Material::getHandlesForShaderResources()
{
	std::vector<ID3D11ShaderResourceView*> result;
	for (std::unique_ptr<Texture2D>& texture : textures)
	{
		result.push_back(texture->shaderResourceViewHandle);
	}

	return result;
}

std::vector<D3D11_INPUT_ELEMENT_DESC> Graphics::Material::createInputLayoutDescFromVertexShaderSignature(ID3DBlob* vertexShaderBlob)
{
	HRESULT result;
	ID3D11ShaderReflection* vertexShaderReflection;
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;

	result = D3DReflect(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), IID_PPV_ARGS(&vertexShaderReflection));

	if (FAILED(result))
	{
		return inputLayoutDesc;
	}

	D3D11_SHADER_DESC shaderDesc;
	vertexShaderReflection->GetDesc(&shaderDesc);

	for (UINT i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		vertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		inputLayoutDesc.push_back(elementDesc);
	}

	vertexShaderReflection->Release();
	return inputLayoutDesc;
}
