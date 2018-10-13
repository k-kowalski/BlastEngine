#pragma once
#include "Vertex.h"
#include "Material.h"
#include <vector>
#include "xnamath.h"

namespace Graphics
{
	struct Model
	{
		std::string name;
		std::vector<Graphics::Vertex> vertices;
		std::vector<UINT> vertexIndices;
		std::unique_ptr<Material> material;

		XMMATRIX objectWorldMatrix;

		ID3D11Buffer* vertexBufferHandle;
		unsigned int vertexBufferCapacity;

		ID3D11Buffer* indexBufferHandle;
		unsigned int indexBufferCapacity;

		Model(std::string name, std::string meshFilePath, std::unique_ptr<Material>& material);
		~Model();
	};
}
