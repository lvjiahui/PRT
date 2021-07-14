#pragma once

#include <fmt/core.h>
#include <imgui.h>
#include <map>

#include "opengl/gl.h"
#include "util/camera.h"
#include "scene/model.h"
#include "sh/volume.h"

class Platform;

class App {
public:
	struct Pixel{
		unsigned char r,g,b,a;
	};
	struct Pixel_accum{
		glm::vec3 color;
		float count;
	};

	static void setup(Platform& plt);
	static App& get();

	void render();

	Platform& plt;
	SH_volume sh_volume{ 8 };
	std::unique_ptr<Model> light_mesh;
	bool multi_bounce = true;
	float atten = 1;
	float cast_light_intensity = 100;
	float cast_light_cut_off = 0.9;
	float cast_light_position[3] = {4, 0, 0};

	Tex2D hdr_RectMap{};
	Tex2D brdfLUT{};
    Tex2D screenTex{};
	std::vector<Pixel> pixels;
	std::vector<Pixel_accum> pixels_w;
	Quad screen_quad{};
	CubeMap& EnvMap(std::string name = {});
	Framebuffer captureFB{};
	Camera camera = Camera{ glm::vec3(0.0f, 0.0f, 3.0f) };
	glm::mat4 Mat_projection{ 1 };
	std::vector<glm::vec3> env_sh{};
	bool tonemap = true;
	bool gamma = true;
	bool white_bk = false;
	float F0[3] = {0.562, 0.565, 0.578};
	// float albedo[3] = {0.5f, 0.05f, 0.05f};
	float albedo[3] = {1.f, 1.f, 1.f};
	bool metal = false;
	bool diffuse = true;
	bool sh = true;
	bool specular = false;
	float lod = 0;
	float roughness = 0.2;
	std::unique_ptr<SkyBox> skybox;
	std::unique_ptr<Model> scene;
	std::unique_ptr<Model> probe_mesh;
	Paral_Shadow sky_shadow;
	float sky_light_pos[2] = {0.38, 0.84};
	float sky_intensity = 1;

	int max_path_length = 2;
	int sh_resolution = 32;

private:
	static inline App* app = nullptr;
	App(Platform& plt);
	void clear();
	void render_imgui();
	void render_3d();

	bool show_demo_window = false;
	bool ray_tracing = false;
	bool rotate_model = false;
	bool rotate_env = false;
	bool render_model = true;
	bool render_SH_probe = false;

	float depth = 1;
	const std::vector<const char*> map_choices = { "environment", "irradiance", "prefilter" };
	std::map<std::string, CubeMap> _CubeMap;
    int map_current = 0;

	ImVec4 clear_color = ImVec4(0.2f, 0.3f, 0.3f, 1.00f);
	std::unique_ptr<Model> model;
	std::unique_ptr<LightProbe> lightProbe;
};