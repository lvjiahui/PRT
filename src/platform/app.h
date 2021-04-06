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
	Tex2D hdr_RectMap;
	Tex2D brdfLUT{};
	CubeMap& EnvMap(std::string name = {});
	Framebuffer captureFB{};
	Camera camera = Camera{ glm::vec3(0.0f, 0.0f, 3.0f) };
	glm::mat4 Mat_projection{ 1 };

	bool tonemap = true;
	bool gamma = true;
	bool white_bk = false;
	float F0[3] = {0.562, 0.565, 0.578};
	float albedo[3] = {0.5f, 0.05f, 0.05f};
	bool metal = false;
	float lod = 0;
	float roughness = 0.2;

private:
	static inline App* data = nullptr;
	App(Platform& plt);
	void clear();
	void render_imgui();
	void render_3d();

	bool show_demo_window = false;
	bool rotate = false;
	bool render_model = true;

	float depth = 1;
	const std::vector<const char*> map_choices = { "environment", "sh_env", "irradiance", "prefilter" };
	std::map<std::string, CubeMap> _CubeMap;
    int map_current = 0;

	ImVec4 clear_color = ImVec4(0.2f, 0.3f, 0.3f, 1.00f);
	std::unique_ptr<Model> model;
	std::unique_ptr<SkyBox> skybox;
	std::unique_ptr<LightProbe> lightProbe;
};