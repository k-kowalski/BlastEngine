#include "Scene.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include "stb_image/stb_image.h"
#include "Diagnostics.h"
#include <array>


void Scene::writeToFile(std::string sceneFile)
{
	HANDLE fileHandle = CreateFileA(sceneFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	// 1. file identifier string - char*
	std::string identifier = "Blast Engine Scene";
	WriteFile(fileHandle, identifier.c_str(), identifier.length() + 1, nullptr, nullptr);

	// 2. models count - int 4
	int modelsCount = models.size(); 
	WriteFile(fileHandle, &modelsCount, sizeof(int), nullptr, nullptr);
	// 3. models - ARRAY
	for (std::unique_ptr<Graphics::Model>& model : models)
	{
		// 4. model name - char*
		WriteFile(fileHandle, model->name.c_str(), model->name.length() + 1, nullptr, nullptr);
		// 4. model vertex count - int 4
		int modelVertexCount = model->vertices.size();
		WriteFile(fileHandle, &modelVertexCount, sizeof(int), nullptr, nullptr);
		// 5. model vertices - float[3], float[2], float[3]
		for (Graphics::Vertex& vertex : model->vertices)
		{
			// write vertex structure
			WriteFile(fileHandle, &vertex.position.x, sizeof(float) * 3, nullptr, nullptr);
			WriteFile(fileHandle, &vertex.UVcoordinates.x, sizeof(float) * 2, nullptr, nullptr);
			WriteFile(fileHandle, &vertex.normal.x, sizeof(float) * 3, nullptr, nullptr);
		}
		// 6. model index count - int 4
		int modelIndexCount = model->vertexIndices.size();
		WriteFile(fileHandle, &modelIndexCount, sizeof(int), nullptr, nullptr);
		// 7. model indices - int 4
		for (UINT index : model->vertexIndices)
		{
			WriteFile(fileHandle, &index, sizeof(UINT), nullptr, nullptr);
		}
		// 8. model transform - float[4][4]
		WriteFile(fileHandle, &model->objectWorldMatrix.m, sizeof(model->objectWorldMatrix.m), nullptr, nullptr);
		// 9. texture count - int 4
		int texturesCount = model->material->textures.size();
		WriteFile(fileHandle, &texturesCount, sizeof(int), nullptr, nullptr);
		// 10. textures - ARRAY
		for (std::unique_ptr<Graphics::Texture2D>& texture : model->material->textures)
		{
			// write texture data
			int textureDataLength = 0;
			unsigned char* compressedTextureData = stbi_write_png_to_mem(texture->data, texture->width * STBI_rgb_alpha, texture->width, texture->height,
				STBI_rgb_alpha,
				&textureDataLength);

			// 12. length of texture data - int
			WriteFile(fileHandle, &textureDataLength, sizeof(int), nullptr, nullptr);
			// 13. texture data
			WriteFile(fileHandle, compressedTextureData, textureDataLength, nullptr, nullptr);
		}
	}

	CloseHandle(fileHandle);
}

Scene::Scene(std::string sceneFile)
{
	HANDLE fileHandle = CreateFileA(sceneFile.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		Diagnostics::messageBoxInfo("Cannot read file");
		return;
	}

	// 1. file identifier string - char*
	std::array<char, 19> identifier;
	ReadFile(fileHandle, identifier.data(), identifier.size(), nullptr, nullptr);
	// printf("%s\n", identifier.data());
	if ( ! std::string(identifier.data())._Equal("Blast Engine Scene"))
	{
		Diagnostics::messageBoxInfo("Error while reading - file identifier is invalid.");
		return;
	}
	// 2. models count - int 4
	int modelsCount = 0;
	ReadFile(fileHandle, &modelsCount, sizeof(int), nullptr, nullptr);
	// printf("models count: %d\n", modelsCount);

	//	3. models - ARRAY
	for (int i = 0; i < modelsCount; ++i)
	{
		// 4. model name - char*
		std::vector<char> modelName;
		char currentChar;
		do 
		{
			ReadFile(fileHandle, &currentChar, sizeof(char), nullptr, nullptr);
			//printf("%c\n", currentChar);
			modelName.push_back(currentChar);
		} while (currentChar != '\0');
		// printf("model name: %s, len: %llu\n", modelName.data(), modelName.size());
		// 4. model vertex count - int 4
		int modelVertexCount = 0;
		ReadFile(fileHandle, &modelVertexCount, sizeof(int), nullptr, nullptr);
		// printf("modelVertexCount: %d\n", modelVertexCount);
		// 5. model vertices - float[3], float[2], float[3]
		std::vector<Graphics::Vertex> vertices;
		for (int j = 0; j < modelVertexCount; ++j)
		{
			Graphics::Vertex v;
			// read vertex structure
			ReadFile(fileHandle, &v.position.x, sizeof(float) * 3, nullptr, nullptr);
			ReadFile(fileHandle, &v.UVcoordinates.x, sizeof(float) * 2, nullptr, nullptr);
			ReadFile(fileHandle, &v.normal.x, sizeof(float) * 3, nullptr, nullptr);

			// printf("vertex pos: x: %f y: %f z: %f\n", v.position.x, v.position.y, v.position.z);
			// printf("vertex UV: x: %f y: %f\n", v.UVcoordinates.x, v.UVcoordinates.y);
			// printf("vertex normal: x: %f y: %f z: %f\n", v.normal.x, v.normal.y, v.normal.z);

			vertices.push_back(v);
		}
		// 6. model index count - int 4
		int modelIndexCount = 0;
		ReadFile(fileHandle, &modelIndexCount, sizeof(int), nullptr, nullptr);
		// 7. model indices - int 4
		std::vector<UINT> vertexIndices;
		for (int j = 0; j < modelIndexCount; ++j)
		{
			UINT index;
			ReadFile(fileHandle, &index, sizeof(UINT), nullptr, nullptr);
			vertexIndices.push_back(index);
		}
		// 8. model transform - float[4][4]
		XMMATRIX transformMatrix;
		ReadFile(fileHandle, &transformMatrix, sizeof(XMMATRIX), nullptr, nullptr);
		// 9. texture count - int 4
		int texturesCount = 0;
		ReadFile(fileHandle, &texturesCount, sizeof(int), nullptr, nullptr);

		// 10. textures - ARRAY
		std::vector<std::unique_ptr<Graphics::Texture2D>> modelTextures;
		for (int j = 0; j < texturesCount; ++j)
		{
			int imageDateLength = 0;
			ReadFile(fileHandle, &imageDateLength, sizeof(int), nullptr, nullptr);
			unsigned char* imageData = new unsigned char[imageDateLength];
			ReadFile(fileHandle, imageData, imageDateLength, nullptr, nullptr);

			modelTextures.push_back(
				std::make_unique<Graphics::Texture2D>(imageDateLength, imageData)
			);

			delete[] imageData;
		}


		std::unique_ptr<Graphics::Material> currentMaterial = std::make_unique<Graphics::Material>(modelTextures, "texture.hlsl");
		std::unique_ptr<Graphics::Model> currentModel = std::make_unique<Graphics::Model>(modelName.data(), vertices, vertexIndices, transformMatrix, currentMaterial);

		models.push_back(std::move(currentModel));
	}
	CloseHandle(fileHandle);
}

Scene::Scene()
{
}

Scene::~Scene()
{
}

Scene& Scene::operator=(Scene&& right)
{
	models = std::move(right.models);
	return *this;
}
