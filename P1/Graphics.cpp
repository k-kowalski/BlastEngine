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
#include <algorithm>
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
		ImGui::StyleColorsLight();
	}

	void shutdownGUI()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();

		ImGui::SaveIniSettingsToDisk("imgui.ini");
		ImGui::DestroyContext();
	}

	void displayTransformation(XMMATRIX& transformationMatrix)
	{
		uint8_t modified = 0;

		XMVECTOR translation;
		XMVECTOR rotation;
		XMVECTOR scale;
		XMMatrixDecompose(&scale, &rotation, &translation, transformationMatrix);

		Math::Vector4 quat;
		quat.x = rotation.m128_f32[0];
		quat.y = rotation.m128_f32[1];
		quat.z = rotation.m128_f32[2];
		quat.w = rotation.m128_f32[3];

		// thanks to LumixEngine
		Math::Vector3 euler;
		float check = 2.0f * (-quat.y * quat.z + quat.w * quat.x);
		if (check < -0.995f)
		{
			euler = Math::Vector3{
				-Math::PI * 0.5f, 0.0f,
				-atan2f(2.0f * (quat.x * quat.z - quat.w * quat.y),
				1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z))};
		}
		else if (check > 0.995f)
		{
			euler = Math::Vector3{
				Math::PI * 0.5f, 0.0f,
				atan2f(2.0f * (quat.x * quat.z - quat.w * quat.y),
				1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z))};
		}
		else
		{
			euler = Math::Vector3{
				asinf(check),
				atan2f(2.0f * (quat.x * quat.z + quat.w * quat.y), 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y)),
				atan2f(2.0f * (quat.x * quat.y + quat.w * quat.z), 1.0f - 2.0f * (quat.x * quat.x + quat.z * quat.z))};
		}

		euler.x = Math::radiansToDegrees(euler.x);
		euler.y = Math::radiansToDegrees(euler.y);
		euler.z = Math::radiansToDegrees(euler.z);

		ImGui::Text("Object transform:");
		ImGui::Columns(4, "transform");
		ImGui::Separator();
		ImGui::Text(""); ImGui::NextColumn();
		ImGui::Text("X"); ImGui::NextColumn();
		ImGui::Text("Y"); ImGui::NextColumn();
		ImGui::Text("Z"); ImGui::NextColumn();
		ImGui::Separator();

		ImGui::Text("Translation"); ImGui::NextColumn();
		modified += ImGui::DragFloat("##TranslationX", &translation.m128_f32[0], 0.005f); ImGui::NextColumn();
		modified += ImGui::DragFloat("##TranslationY", &translation.m128_f32[1], 0.005f); ImGui::NextColumn();
		modified += ImGui::DragFloat("##TranslationZ", &translation.m128_f32[2], 0.005f); ImGui::NextColumn();
		ImGui::Separator();

		ImGui::Text("Rotation"); ImGui::NextColumn();
		modified += ImGui::DragFloat("##RotationX", &euler.x, 0.01f); ImGui::NextColumn();
		modified += ImGui::DragFloat("##RotationY", &euler.y, 0.01f); ImGui::NextColumn();
		modified += ImGui::DragFloat("##RotationZ", &euler.z, 0.01f); ImGui::NextColumn();
		ImGui::Separator();

		ImGui::Text("Scale"); ImGui::NextColumn();		
		modified += ImGui::DragFloat("##ScaleX", &scale.m128_f32[0], 0.005f); ImGui::NextColumn();
		modified += ImGui::DragFloat("##ScaleY", &scale.m128_f32[1], 0.005f); ImGui::NextColumn();
		modified += ImGui::DragFloat("##ScaleZ", &scale.m128_f32[2], 0.005f); ImGui::NextColumn();
		ImGui::Separator();


		if (modified > 0)
		{
			if (euler.x <= -90.0f || euler.x >= 90.0f)
			{		
				euler.y = 0;
			}
			euler.x = Math::degreesToRadians(std::clamp(euler.x, -90.0f, 90.0f));
			euler.y = Math::degreesToRadians(fmodf(euler.y + 180, 360.0f) - 180);
			euler.z = Math::degreesToRadians(fmodf(euler.z + 180, 360.0f) - 180);

			transformationMatrix = XMMatrixIdentity() * XMMatrixScalingFromVector(scale) * XMMatrixRotationRollPitchYaw(euler.x, euler.y, euler.z) * XMMatrixTranslationFromVector(translation);
		}
	}

	void runGUI()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		
		if (ImGui::Begin("Control panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (ImGui::Button("Load scene from file"))
			{
				currentScene = Scene(R"(..\data\1.scene)");
			}
			ImGui::SameLine();
			if (ImGui::Button("Save scene to file"))
			{
				currentScene.WriteToFile(R"(..\data\1.scene)");
			}

			if (ImGui::Button("Init scene"))
			{
				// plane
				std::vector<std::unique_ptr<Graphics::Texture2D>> cubeTextures;
				cubeTextures.push_back(
					std::make_unique<Graphics::Texture2D>(R"(..\data\textures\grass.jpg)")
				);

				std::unique_ptr<Graphics::Material> cubeMaterial = std::make_unique<Graphics::Material>(cubeTextures, "texture.hlsl");
				std::unique_ptr<Graphics::Model> cubeModel = std::make_unique<Graphics::Model>("plane", R"(..\data\models\plane.fbx)", cubeMaterial);

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

				fenceModel->objectWorldMatrix *= XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 3.0f);

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

				rockModel->objectWorldMatrix *= XMMatrixTranslation(2.0f, -3.0f, -0.5f);

				currentScene.models.push_back(
					std::move(rockModel)
				);
			}
			if (ImGui::Button("Toggle wireframe mode"))
			{
				Graphics::toggleWireframeRendering();
			}			
		}
		ImGui::End();

		// scene
		if (ImGui::Begin("Scene", nullptr))
		{
			static int32_t selected;

			ImGui::ListBox("", &selected, [](void* data, int index, const char** outText)
			{
				auto& models = *static_cast<std::vector<std::unique_ptr<Graphics::Model>>*>(data);
				if (index < 0 || index >= static_cast<int>(models.size())) { return false; }

				*outText = models.at(index)->name.c_str();

				return true;

			},static_cast<void*>(&currentScene.models), currentScene.models.size());

			if ( ! currentScene.models.empty())
			{
				auto& selectedModel = currentScene.models[selected];

				// std::array<const char*, 4> transformColumnNames = {
				// 	"", "Translation", "Rotation", "Scale"
				// };

				displayTransformation(selectedModel->objectWorldMatrix);
			}	


			// if (ImGui::Button("Translate"))
			// {
			// 	currentScene.models[selected]->objectWorldMatrix *= XMMatrixTranslation(0.3f, 0.0f, 0.0f);
			// }
		}
		ImGui::End();

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



	struct ConstantBufferPerObject
	{
		XMMATRIX worldViewProjectionMatrix;
	};
	ConstantBufferPerObject cbPerObject;
	
	// struct ConstantBufferCamera
	// {
	// 	Math::Vector3 target;
	// };
	// ConstantBufferCamera cbCamera;

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

	void resizeBuffers(HWND renderWindowHandle)
	{
		graphicsDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
		renderTargetView->Release();

		HRESULT result;
		// Preserve the existing buffer count and format.
		// Automatically choose the width and height to match the client rect for HWNDs.
		result = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);


		ID3D11Texture2D* buffer;
		result = swapChain->GetBuffer(0, IID_PPV_ARGS(&buffer));

		result = graphicsDevice->CreateRenderTargetView(buffer, nullptr, &renderTargetView);
		buffer->Release();

		depthStencilView->Release();
		D3D11_TEXTURE2D_DESC depthStencilDesc;
		depthStencilDesc.Width = Platform::getClientSpaceWidth(renderWindowHandle);
		depthStencilDesc.Height = Platform::getClientSpaceHeight(renderWindowHandle);
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
		viewport.Width = static_cast<float>(depthStencilDesc.Width);
		viewport.Height = static_cast<float>(depthStencilDesc.Height);
		viewport.MinDepth = 0.0;
		viewport.MaxDepth = 1.0;

		graphicsDeviceContext->RSSetViewports(1, &viewport);
	}

	bool initializeScene()
	{
//		loadGeometry(sampleGeometryFile);
//		loadScene(R"(D:\COMMON\pracownia\Blenderpracownia\environment\txt\fence_1.png)");

		graphicsDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D11_BUFFER_DESC constantBufferDesc;
		ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));
		constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		constantBufferDesc.ByteWidth = sizeof(ConstantBufferPerObject);
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

		cbPerObject.worldViewProjectionMatrix = XMMatrixTranspose(XMMatrixIdentity() * camView * camProjection);

		graphicsDeviceContext->UpdateSubresource(cbPerObjectBuffer, 0, nullptr, &cbPerObject, 0, 0);

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
		cbPerObject.worldViewProjectionMatrix = XMMatrixTranspose(model->objectWorldMatrix * camView * camProjection);
		graphicsDeviceContext->UpdateSubresource(cbPerObjectBuffer, 0, nullptr, &cbPerObject, 0, 0);
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
