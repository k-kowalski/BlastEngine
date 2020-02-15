#pragma once
#include "Model.h"

class Scene
{
public:
	static constexpr const char* SCENE_FILE_IDENTIFIER = "Blast Engine Scene";
	std::vector<std::unique_ptr<Graphics::Model>> models;

	Scene(std::string sceneFile);
	Scene() = default;
	~Scene() = default;

	void WriteToFile(std::string sceneFile);

	Scene& operator=(Scene&& right);
};

