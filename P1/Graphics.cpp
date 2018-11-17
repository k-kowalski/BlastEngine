#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include "xnamath.h"
#include <stdio.h>
#include "Graphics.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "Platform.h"
#include "Diagnostics.h"
#include <set>
#include <memory>
#include "Model.h"
#include "Scene.h"

Scene currentScene;
constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

namespace GUI
{
	void initializeGUI(HWND renderWindowHandle, ID3D11Device* graphicsDevice, ID3D11DeviceContext* graphicsDeviceContext)
	{
		ImGui::CreateContext();
		ImGui::LoadIniSettingsFromDisk("imgui.ini");
		ImGui::GetIO().IniFilename = nullptr;

		ImGui_ImplWin32_Init(renderWindowHandle);
		ImGui_ImplDX11_Init(graphicsDevice, graphicsDeviceContext);
		ImGui::StyleColorsClassic();
	}

	void shutdownGUI()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();

		ImGui::SaveIniSettingsToDisk("imgui.ini");
		ImGui::DestroyContext();
	}

	void runGUI()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Control panel");
		if (ImGui::Button("Load scene from file"))
		{
			currentScene = Scene(R"(D:\COMMON\pracownia\VisualStudio\BlastEngine\data\1.scene)");
		}

		if (ImGui::Button("Dump scene to file"))
		{
			currentScene.writeToFile(R"(D:\COMMON\pracownia\VisualStudio\BlastEngine\data\1.scene)");
		}

		if (ImGui::Button("Init scene"))
		{
			// cube
			std::vector<std::unique_ptr<Graphics::Texture2D>> cubeTextures;
			cubeTextures.push_back(
				std::make_unique<Graphics::Texture2D>(R"(..\data\textures\grass.jpg)")
			);

			std::unique_ptr<Graphics::Material> cubeMaterial = std::make_unique<Graphics::Material>(cubeTextures, "texture.hlsl");
			std::unique_ptr<Graphics::Model> cubeModel =  std::make_unique<Graphics::Model>("plane", R"(..\data\models\plane.fbx)", cubeMaterial);

			cubeModel->objectWorldMatrix *= XMMatrixTranslation(0.0f, 0.0f, -1.2f);
			cubeModel->objectWorldMatrix *= XMMatrixScaling(10.0f, 10.0f, 1.0f);

			currentScene.models.push_back(
				std::move(cubeModel)
			);


			// fence
			std::vector<std::unique_ptr<Graphics::Texture2D>> fenceTextures;
			fenceTextures.push_back(
				std::make_unique<Graphics::Texture2D>(R"(..\data\textures\fence_1.png)")
			);

			std::unique_ptr<Graphics::Material> fenceMaterial = std::make_unique<Graphics::Material>(fenceTextures, "texture.hlsl");

			std::unique_ptr<Graphics::Model> fenceModel = std::make_unique<Graphics::Model>("fence", R"(..\data\models\fence_1.fbx)", fenceMaterial);
			currentScene.models.push_back(
				std::move(fenceModel)
			);
			// rock
			std::vector<std::unique_ptr<Graphics::Texture2D>> rockTextures;
			rockTextures.push_back(
				std::make_unique<Graphics::Texture2D>(R"(..\data\textures\rock1.png)")
			);

			std::unique_ptr<Graphics::Material> rockMaterial = std::make_unique<Graphics::Material>(rockTextures, "texture.hlsl");

			std::unique_ptr<Graphics::Model> rockModel = std::make_unique<Graphics::Model>("rock", R"(..\data\models\rock_1.fbx)", rockMaterial);

			rockModel->objectWorldMatrix *= XMMatrixTranslation(1.0f, -3.0f, -0.5f);

			currentScene.models.push_back(
				std::move(rockModel)
			);
		}
		if (ImGui::Button("Toggle wireframe mode"))
		{
			Graphics::toggleWireframeRendering();
		}
		if (ImGui::Button("Move rock!"))
		{
			for (auto& model : currentScene.models)
			{
				if (model->name._Equal("rock"))
				{
					model->objectWorldMatrix *= XMMatrixTranslation(0.3f, 0.0f, 0.0f);
					break;
				}
			}
		}
		ImGui::End();

//		ImGui::Begin("Details");
//		std::array<char, 128> fileMessage;
//		sprintf_s(fileMessage.data(), fileMessage.size(), "file:\n%s\n", currentGeometrySourceName.c_str());
//		ImGui::Text(fileMessage.data());
//		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

}

namespace Graphics
{
	XMVECTOR DefaultForward = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR DefaultRight = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR DefaultUpward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR camForward = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR camRight = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR camUpward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	XMMATRIX camRotationMatrix;
	XMMATRIX groundWorld;

	float camYaw;
	float camPitch;

	float moveLeftRight;
	float moveBackForward;
	float moveDownwardUpward;

	XMMATRIX camView;
	XMMATRIX camProjection;

	XMVECTOR camPosition = XMVectorSet(-2.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR camTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR camUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	IDXGISwapChain* swapChain;
	ID3D11Device* graphicsDevice;
	ID3D11DeviceContext* graphicsDeviceContext;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	ID3D11Texture2D* depthStencilBuffer;
	ID3D11RasterizerState* rasterizerState;
	ID3D11ShaderResourceView* texture;

	ID3D11InputLayout* vertexInputLayout;

	ID3D11Buffer* vertexBuffer;
	UINT vertexBufferCapacity = 0;

	ID3D11Buffer* indexBuffer;
	UINT currentIndexCount;
	UINT indexBufferCapacity = 0;


	Camera* defCam;
	ID3D11Buffer* cbPerObjectBuffer;
	XMMATRIX worldViewProjectionMatrix;
	XMMATRIX worldMatrix;


	XMMATRIX objectWorldMatrix;

	XMMATRIX Rotation;
	XMMATRIX Scale;
	XMMATRIX Translation;
	float rot = 0.01f;



	struct cbPerObject
	{
		XMMATRIX worldViewProjectionMatrix;
	};
	cbPerObject cbPerObj;

	bool initializeGraphics(HWND renderWindowHandle)
	{
		HRESULT result;
		DXGI_MODE_DESC backBufferDesc;
		ZeroMemory(&backBufferDesc, sizeof(DXGI_MODE_DESC));
		backBufferDesc.Width = Platform::getClientSpaceWidth(renderWindowHandle);
		backBufferDesc.Height = Platform::getClientSpaceHeight(renderWindowHandle);
		backBufferDesc.RefreshRate.Numerator = 60;
		backBufferDesc.RefreshRate.Denominator = 1;
		backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		backBufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		backBufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDesc.BufferDesc = backBufferDesc;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.OutputWindow = renderWindowHandle;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
			D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &graphicsDevice, nullptr, &graphicsDeviceContext);

		ID3D11Texture2D* backBuffer;
		result = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

		result = graphicsDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
		backBuffer->Release();

		D3D11_TEXTURE2D_DESC depthStencilDesc;
		depthStencilDesc.Width = backBufferDesc.Width;
		depthStencilDesc.Height = backBufferDesc.Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;
		graphicsDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);
		graphicsDevice->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView);

		graphicsDeviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

		viewport.TopLeftX = 0.0;
		viewport.TopLeftY = 0.0;
		viewport.Width = static_cast<float>(backBufferDesc.Width);
		viewport.Height = static_cast<float>(backBufferDesc.Height);
		viewport.MinDepth = 0.0;
		viewport.MaxDepth = 1.0;

		graphicsDeviceContext->RSSetViewports(1, &viewport);

		camProjection = XMMatrixPerspectiveFovRH(0.4f * 3.14f, viewport.Width / viewport.Height, 0.1f, 1000.0f);

		/* GUI setup */
		GUI::initializeGUI(renderWindowHandle, graphicsDevice, graphicsDeviceContext);

		// if succeded, proceed with loading default scene
		return (result == 0) ? initializeScene() : false;
	}

	void shutdownGraphics()
	{
		GUI::shutdownGUI();

		swapChain->Release();
		graphicsDevice->Release();
		graphicsDeviceContext->Release();
		renderTargetView->Release();
		depthStencilView->Release();
		depthStencilBuffer->Release();
		rasterizerState->Release();

		if (vertexBuffer != nullptr)
		{
			vertexBuffer->Release();
		}
		if (indexBuffer != nullptr)
		{
			indexBuffer->Release();
		}
		cbPerObjectBuffer->Release();

		delete defCam;
	}

	bool initializeScene()
	{
//		loadGeometry(sampleGeometryFile);
//		loadScene(R"(D:\COMMON\pracownia\Blenderpracownia\environment\txt\fence_1.png)");

		graphicsDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D11_BUFFER_DESC constantBufferDesc;
		ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));
		constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		constantBufferDesc.ByteWidth = sizeof(cbPerObject);
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constantBufferDesc.CPUAccessFlags = 0;
		constantBufferDesc.MiscFlags = 0;

		graphicsDevice->CreateBuffer(&constantBufferDesc, nullptr, &cbPerObjectBuffer);

		D3D11_RASTERIZER_DESC rasterizerStateDesc;
		ZeroMemory(&rasterizerStateDesc, sizeof(D3D11_RASTERIZER_DESC));
		rasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerStateDesc.CullMode = D3D11_CULL_NONE;
		graphicsDevice->CreateRasterizerState(&rasterizerStateDesc, &rasterizerState);
		graphicsDeviceContext->RSSetState(rasterizerState);

		cbPerObj.worldViewProjectionMatrix = XMMatrixTranspose(XMMatrixIdentity() * camView * camProjection);

		graphicsDeviceContext->UpdateSubresource(cbPerObjectBuffer, 0, nullptr, &cbPerObj, 0, 0);

		graphicsDeviceContext->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);

		//moveCameraUpward();

		// setup object and camera
		//objectWorldMatrix = XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0);
		//Scale = worldScale;

		//		moveDownwardUpward = 1.0f;
		//		moveBackForward = -7.0f;
		updateCamera();

		return true;
	}

	void toggleWireframeRendering()
	{
		D3D11_RASTERIZER_DESC rasterizerStateDesc;
		rasterizerState->GetDesc(&rasterizerStateDesc);

		if (rasterizerStateDesc.FillMode == D3D11_FILL_WIREFRAME)
		{
			rasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
		}
		else
		{
			rasterizerStateDesc.FillMode = D3D11_FILL_WIREFRAME;
		}

		graphicsDevice->CreateRasterizerState(&rasterizerStateDesc, &rasterizerState);
		graphicsDeviceContext->RSSetState(rasterizerState);
	}

	HWND getRenderWindowHandle()
	{
		if (swapChain == nullptr)
		{
			return nullptr;
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		swapChain->GetDesc(&swapChainDesc);
		return swapChainDesc.OutputWindow;
	}

	void setMaterial(Material* material)
	{
		graphicsDeviceContext->IASetInputLayout(material->vertexInputLayout);
		graphicsDeviceContext->VSSetShader(material->vertexShader, 0, 0);
		graphicsDeviceContext->PSSetShader(material->pixelShader, 0, 0);
		graphicsDeviceContext->PSSetSamplers(0, 1, &material->samplerState);

		std::vector<ID3D11ShaderResourceView*> shaderResourceHandles = material->getHandlesForShaderResources();
		graphicsDeviceContext->PSSetShaderResources(0, static_cast<UINT>(shaderResourceHandles.size()), &shaderResourceHandles[0]);
	}

	ID3D11Device* getGraphicsDevice()
	{
		return graphicsDevice;
	}

	void renderModel(Model* model)
	{
		// transformation
		cbPerObj.worldViewProjectionMatrix = XMMatrixTranspose(model->objectWorldMatrix * camView * camProjection);
		graphicsDeviceContext->UpdateSubresource(cbPerObjectBuffer, 0, nullptr, &cbPerObj, 0, 0);
		graphicsDeviceContext->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);

		// material update
		setMaterial(model->material.get());

		// geometry update
		unsigned int stride = sizeof(Vertex);
		unsigned int offset = 0;
		graphicsDeviceContext->IASetVertexBuffers(0, 1, &model->vertexBufferHandle, &stride, &offset);
		graphicsDeviceContext->IASetIndexBuffer(model->indexBufferHandle, DXGI_FORMAT_R32_UINT, 0);

		// draw
		graphicsDeviceContext->DrawIndexed(
			static_cast<unsigned int>(model->vertexIndices.size()),
			0, 0);
	}

	void updateScene()
	{
		//		rot += .01f;
		//		if (rot > 6.28f)
		//			rot = 0.0f;

		//		XMVECTOR rotaxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		//		Rotation = XMMatrixRotationAxis(rotaxis, -rot);

		//objectWorldMatrix = Scale;
	}

	void drawScene()
	{
		graphicsDeviceContext->ClearRenderTargetView(renderTargetView, clearColor);
		graphicsDeviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);

		for (std::unique_ptr<Model>& model : currentScene.models)
		{
			renderModel(model.get());
		}

		GUI::runGUI();

		swapChain->Present(1, 0);
	}

	void modifyScale(float factor)
	{
		Scale *= XMMatrixScaling(factor, factor, factor);
	}

	void updateCameraAngles(int x, int y)
	{
		camYaw -= x * 0.005f;
		camPitch += y * 0.005f;
		updateCamera();
	}

	void updateCamera()
	{
		//printf("y: %f | p: %f\n", camYaw, camPitch);

		// lock on 90 degree pitch
		if (camPitch > 1.57f)
		{
			camPitch = 1.57f;
		}
		else if (camPitch < -1.57f)
		{
			camPitch = -1.57f;
		}

		auto y = XMMatrixRotationRollPitchYaw(0, camPitch, 0);
		auto p = XMMatrixRotationRollPitchYaw(0, 0, camYaw);
		camRotationMatrix = y * p;
		camTarget = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
		camTarget = XMVector3Normalize(camTarget);

		XMMATRIX rotateMat = XMMatrixRotationZ(camYaw);

		camRight = XMVector3TransformCoord(DefaultRight, rotateMat);
		camUp = XMVector3TransformCoord(camUp, rotateMat);
		camForward = XMVector3TransformCoord(DefaultForward, rotateMat);

		camPosition += moveLeftRight * camRight;
		camPosition += moveBackForward * camForward;
		camPosition += moveDownwardUpward * camUpward;

		moveLeftRight = 0.0f;
		moveBackForward = 0.0f;
		moveDownwardUpward = 0.0f;

		camTarget = camPosition + camTarget;

		camView = XMMatrixLookAtRH(camPosition, camTarget, camUp);
	}

	void moveCameraRight()
	{
		moveLeftRight = -0.1f;
		updateCamera();
	}

	void moveCameraLeft()
	{
		moveLeftRight = 0.1f;
		updateCamera();
	}

	void moveCameraForward()
	{
		moveBackForward = 0.1f;
		updateCamera();
	}

	void moveCameraBackward()
	{
		moveBackForward = -0.1f;
		updateCamera();
	}

	void moveCameraUpward()
	{
		moveDownwardUpward = 0.1f;
		updateCamera();
	}

	void moveCameraDownward()
	{
		moveDownwardUpward = -0.1f;
		updateCamera();
	}
}
