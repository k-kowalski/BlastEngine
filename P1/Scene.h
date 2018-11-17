#pragma once
#include "Model.h"

struct Scene
{
	std::vector<std::unique_ptr<Graphics::Model>> models;

	void writeToFile(std::string sceneFile);

	Scene(std::string sceneFile);
	Scene();
	~Scene();

	Scene& operator=(Scene&& right);
};

