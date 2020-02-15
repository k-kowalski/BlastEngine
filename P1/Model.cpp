#include "Model.h"
#include "OpenFBX/ofbx.h"
#include "Diagnostics.h"
#include <algorithm>
#include <set>
#include "Graphics.h"

Graphics::Model::Model(std::string name, std::string meshFilePath, std::unique_ptr<Material>& material)
{
	this->name = name;
	this->material = std::move(material);
	this->objectWorldMatrix = XMMatrixIdentity();

	if (meshFilePath.compare("") == 0 || meshFilePath.find(".fbx") == std::string::npos)
	{
		Diagnostics::messageBoxInfo("Invalid file.");
		return;
	}

	HANDLE fileHandle = CreateFileA(meshFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	DWORD fileSize = GetFileSize(fileHandle, nullptr);
	ofbx::u8* content = new ofbx::u8[fileSize];
	DWORD read = 0;
	ReadFile(fileHandle, content, fileSize, &read, nullptr);

	ofbx::IScene* scene = ofbx::load(content, fileSize);
	UINT meshCount = scene->getMeshCount();

	unsigned short currentIndex = 0;

	// vertex - index
	std::set<std::pair<Vertex*, unsigned short>> uniqueVertexIndex;

	for (UINT i = 0; i < meshCount; ++i)
	{
		const ofbx::Vec3* meshVertices = scene->getMesh(i)->getGeometry()->getVertices();
		const ofbx::Vec2* meshUVs = scene->getMesh(i)->getGeometry()->getUVs();
		const ofbx::Vec3* meshNormals = scene->getMesh(i)->getGeometry()->getNormals();

		UINT vertexCount = scene->getMesh(i)->getGeometry()->getVertexCount();

		Math::Vector2 currentUVcoords = { -1, -1 };
		for (UINT j = 0; j < vertexCount; ++j)
		{

			if (meshUVs != nullptr)
			{
				currentUVcoords = { static_cast<float>(meshUVs[j].x),  1 - static_cast<float>(meshUVs[j].y) };
			}

			// compare attributes of a vertex
			auto pos = std::find_if(uniqueVertexIndex.begin(), uniqueVertexIndex.end(),
				[meshVertices, currentUVcoords, meshNormals, j](std::pair<Vertex*, UINT> pair)
			{
				return
					// determine vertex identity by comparing
					// position
					Math::compareReal(pair.first->position.x, meshVertices[j].x) &&
					Math::compareReal(pair.first->position.y, meshVertices[j].y) &&
					Math::compareReal(pair.first->position.z, meshVertices[j].z) &&
					// UVs
					Math::compareReal(pair.first->uv.x, currentUVcoords.x) &&
					Math::compareReal(pair.first->uv.y, currentUVcoords.y) &&
					// normals
					Math::compareReal(pair.first->normal.x, meshNormals[j].x) &&
					Math::compareReal(pair.first->normal.y, meshNormals[j].y) &&
					Math::compareReal(pair.first->normal.z, meshNormals[j].z);
			});

			//printf("processing: x:%f y:%f z:%f\n", meshVertices[j].x, meshVertices[j].y, meshVertices[j].z);
			// check whether vertex is unique in collection
			if (pos == uniqueVertexIndex.end())
			{
				// printf("%f %f\n", currentUVcoords.x, currentUVcoords.y);

				// if not, register new vertex
				Vertex newVertex = Vertex
				{
					{
						static_cast<float>(meshVertices[j].x),
						static_cast<float>(meshVertices[j].y),
						static_cast<float>(meshVertices[j].z)
					},
					{
						static_cast<float>(currentUVcoords.x),
						static_cast<float>(currentUVcoords.y)
					},
					{
						static_cast<float>(meshNormals[j].x),
						static_cast<float>(meshNormals[j].y),
						static_cast<float>(meshNormals[j].z)
					}
				};
				vertexIndices.push_back(currentIndex);
				vertices.push_back(newVertex);
				uniqueVertexIndex.insert({ &newVertex, currentIndex++ });
			}
			else
			{
				// if yes, reuse index
				vertexIndices.push_back(pos->second);
			}
		}
	}
//	printf("%s\n", name.c_str());

//	for (auto element : vertices)
//	{
//		printf("x:%f y:%f z:%f\n", element.position.x, element.position.y, element.position.z);
//	}
//
//	for (auto element : vertexIndices)
//	{
//		printf("x:%d\n", element);
//	}

	scene->destroy();
	delete[] content;
	CloseHandle(fileHandle);

	// TODO:
	// move GPU resources setup to just before displaying on specific device
	setupGPUResources(Graphics::getGraphicsDevice());
}

Graphics::Model::Model(std::string name, std::vector<Graphics::Vertex> vertices, std::vector<UINT> vertexIndices, XMMATRIX objectWorldMatrix, std::unique_ptr<Material>& material)
{
	this->name = name;
	this->material = std::move(material);
	this->objectWorldMatrix = objectWorldMatrix;
	this->vertices = vertices;
	this->vertexIndices = vertexIndices;

	// TODO:
	// move GPU resources setup to just before displaying on specific device
	setupGPUResources(Graphics::getGraphicsDevice());
}

Graphics::Model::~Model()
{

}

void Graphics::Model::setupGPUResources(ID3D11Device* graphicsDevice)
{

	// vertex buffer update
	unsigned int vertexBufferSize = static_cast<unsigned int>(vertices.size()) * sizeof(Vertex);
	vertexBufferCapacity = vertexBufferSize;

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = vertexBufferCapacity;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = vertices.data();

	graphicsDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBufferHandle);

	// index buffer update
	unsigned int indexBufferSize = static_cast<unsigned int>(vertexIndices.size()) * sizeof(UINT);
	indexBufferCapacity = indexBufferSize;

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
	indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	indexBufferDesc.ByteWidth = indexBufferCapacity;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	ZeroMemory(&indexBufferData, sizeof(indexBufferData));
	indexBufferData.pSysMem = vertexIndices.data();

	graphicsDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &indexBufferHandle);
}
