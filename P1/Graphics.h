#pragma once

#include "Windows.h"
#include <d3d11_4.h>
#include "Math.h"
#include "Material.h"
#include "Model.h"

namespace Graphics
{
	struct Camera
	{
		Math::Vector3 position;
		Math::Vector3 target;

		float pitch, yaw, roll;

		//DirectX::XMMATRIX* rotationMatrix;
	};

	bool initializeGraphics(HWND renderWindowHandle);
	void shutdownGraphics();

	bool initializeScene();
	void updateScene();
	void drawScene();

	void modifyScale(float factor);

	void updateCameraAngles(int x, int y);
	void updateCamera();
	void moveCameraRight();
	void moveCameraLeft();
	void moveCameraForward();
	void moveCameraBackward();
	void moveCameraUpward();
	void moveCameraDownward();

	void toggleWireframeRendering();

	HWND getRenderWindowHandle();

	void setMaterial(Material* material);

	ID3D11Device* getGraphicsDevice();

	void loadScene(std::string fileName);
	void renderModel(Model* model);
}
