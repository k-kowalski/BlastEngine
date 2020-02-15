#include "Scene.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include "stb_image/stb_image.h"
#include "Diagnostics.h"
#include <array>
#include <fstream>
#include <iostream>
#include "Util/File.h"


Scene::Scene(std::string sceneFileName)
{
	File sceneFile(sceneFileName);

	// 1. file identifier string - char*
	std::string sceneIdentifier;
	sceneFile >> sceneIdentifier;

	if ( ! sceneIdentifier._Equal(SCENE_FILE_IDENTIFIER))
	{
		Diagnostics::messageBoxInfo("Error while reading - file identifier is invalid.");
		return;
	}
	
	// 2. models count
	std::size_t modelsCount = 0;
	sceneFile >> modelsCount;

	//	3. models
	for (std::size_t i = 0; i < modelsCount; ++i)
	{
		// 4.1. model name
		std::string modelName;
		sceneFile >> modelName;
		
		// 4.2. model vertex count
		std::size_t modelVertexCount = 0;
		sceneFile >> modelVertexCount;
		
		// 5. model vertices
		std::vector<Graphics::Vertex> vertices;
		for (std::size_t j = 0; j < modelVertexCount; ++j)
		{
			Graphics::Vertex vertex = {};

			sceneFile >> vertex.position.x >> vertex.position.y >> vertex.position.z;
			sceneFile >> vertex.uv.x >> vertex.uv.y;
			sceneFile >> vertex.normal.x >> vertex.normal.y >> vertex.normal.z;

			vertices.push_back(vertex);
		}
		
		// 6. model index count
		std::size_t modelIndexCount = 0;
		sceneFile >> modelIndexCount;
		
		// 7. model indices
		std::vector<uint32_t> vertexIndices;
		for (std::size_t j = 0; j < modelIndexCount; ++j)
		{
			uint32_t vertexIndex = 0;
			sceneFile >> vertexIndex;
			vertexIndices.push_back(vertexIndex);
		}
		// 8. model transform
		XMMATRIX transformMatrix;
		sceneFile.Read(&transformMatrix, sizeof(transformMatrix));
		
		// 9. texture count
		std::size_t texturesCount = 0;
		sceneFile >> texturesCount;

		// 10. textures
		std::vector<std::unique_ptr<Graphics::Texture2D>> modelTextures;
		for (std::size_t j = 0; j < texturesCount; ++j)
		{
			int32_t textureDataLength = 0;
			sceneFile >> textureDataLength;
			
			unsigned char* textureData = new unsigned char[textureDataLength];
			sceneFile.Read(textureData, textureDataLength);

			modelTextures.push_back(
				std::make_unique<Graphics::Texture2D>(textureDataLength, textureData)
			);

			delete[] textureData;
		}

		std::unique_ptr<Graphics::Material> currentMaterial = std::make_unique<Graphics::Material>(modelTextures, "texture.hlsl");
		std::unique_ptr<Graphics::Model> currentModel = std::make_unique<Graphics::Model>(modelName.data(), vertices, vertexIndices, transformMatrix, currentMaterial);

		models.push_back(std::move(currentModel));
	}
}

void Scene::WriteToFile(std::string sceneFileName)
{
	File sceneFile(sceneFileName);

	// 1. file identifier string
	sceneFile << std::string(SCENE_FILE_IDENTIFIER);

	// 2. models count
	sceneFile << models.size();

	// 3. models
	for (std::unique_ptr<Graphics::Model>& model : models)
	{
		// 4.1 model name
		sceneFile << model->name;
		// 4.2 model vertex count
		sceneFile << model->vertices.size();
		// 5. model vertices
		for (Graphics::Vertex& vertex : model->vertices)
		{
			sceneFile << vertex.position.x << vertex.position.y << vertex.position.z;
			sceneFile << vertex.uv.x << vertex.uv.y;
			sceneFile << vertex.normal.x << vertex.normal.y << vertex.normal.z;
		}
		// 6. model index count
		sceneFile << model->vertexIndices.size();
		// 7. model indices
		for (uint32_t index : model->vertexIndices)
		{
			sceneFile << index;
		}
		// 8. model transform
		sceneFile.Write(&model->objectWorldMatrix, sizeof(model->objectWorldMatrix));
		// 9. texture count
		sceneFile << model->material->textures.size();
		// 10. textures
		for (std::unique_ptr<Graphics::Texture2D>& texture : model->material->textures)
		{
			int32_t textureDataLength = 0;
			unsigned char* compressedTextureData = stbi_write_png_to_mem(texture->data, texture->width * STBI_rgb_alpha,
			                                                          texture->width, texture->height,
			                                                          STBI_rgb_alpha,
			                                                          &textureDataLength);

			// 12. length of texture data
			sceneFile << textureDataLength;
			// 13. texture data
			sceneFile.Write(compressedTextureData, textureDataLength);
		}
	}
}

Scene& Scene::operator=(Scene&& right)
{
	models = std::move(right.models);
	return *this;
}
