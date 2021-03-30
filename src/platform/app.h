#pragma once

#include <fmt/core.h>
#include <imgui.h>
#include <map>

#include "opengl/gl.h"
#include "util/camera.h"
#include "scene/model.h"

class Platform;

class App {
public:
	static void setup(Platform& plt);
	static App& get();

	void render();

	Platform& plt;
	GLuint hdr_RectMap;
	GLuint CubeMap(std::string name = {});

	Camera camera = Camera{ glm::vec3(0.0f, 0.0f, 3.0f) };
	glm::mat4 Mat_projection{ 1 };

	bool tonemap = true;
	bool gamma = true;

private:
	static inline App* data = nullptr;
	App(Platform& plt);
	void clear();
	void render_imgui();
	void render_3d();

	bool show_demo_window = false;
	bool rotate = false;
	float depth = 1;
	
	const std::vector<const char*> map_choices = { "skyMap", "irradiance" };
	std::map<std::string, GLuint> _CubeMap;
    int map_current = 1;

	ImVec4 clear_color = ImVec4(0.2f, 0.3f, 0.3f, 1.00f);
	std::unique_ptr<Model> model;
	std::unique_ptr<SkyBox> sky_box;
};