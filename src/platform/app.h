#pragma once

#include <fmt/core.h>
#include <imgui.h>

#include "gl.h"
#include "util/camera.h"
#include "scene/model.h"

class Platform;

class App {
public:
	App(Platform& plt, Shader&& shader);
	Shader shader;
	void render();
	Camera camera = Camera{ Vec3(0.0f, 0.0f, 3.0f) };

private:
	void clear();
	void render_imgui();
	void render_3d();

	bool show_demo_window = false;
	bool rotate = true;
	float depth = 1;
	ImVec4 clear_color = ImVec4(0.2f, 0.3f, 0.3f, 1.00f);
	Platform& plt;
	std::unique_ptr<Model> model;
	GLuint cubemapTexture;
};