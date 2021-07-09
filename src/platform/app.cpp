#include "app.h"
#include "platform.h"
#include "util/util.h"
#include <iostream>
#include <sf_libs/stb_image.h>

App::App(Platform& plt)
	: plt(plt) {
	pixels.resize(plt.SCR_WIDTH * plt.SCR_HEIGHT);
	pixels_w.resize(plt.SCR_WIDTH * plt.SCR_HEIGHT);
	model = std::make_unique<Model>("data/buddha.obj");
	scene = std::make_unique<Model>("data/cube.obj");
	probe_mesh = std::make_unique<Model>("data/probe.obj");
	skybox = std::make_unique<SkyBox>();
	lightProbe = std::make_unique<LightProbe>(*skybox);
}

void App::setup(Platform& plt)
{
	app = new App(plt);

	Shaders::brdfShader = Shader{ fs::path{"src/opengl/brdf.vert"}, fs::path{"src/opengl/brdf.frag"} };
	Shaders::screenShader = Shader{ fs::path{"src/opengl/screen.vert"}, fs::path{"src/opengl/screen.frag"} };
	Shaders::envShader = Shader{ fs::path{"src/opengl/mesh.vert"}, fs::path{"src/opengl/mesh.frag"} };
	Shaders::castlightShader = Shader{ fs::path{"src/opengl/mesh.vert"}, fs::path{"src/opengl/castlight.frag"} };

	auto& lightProbe = app->lightProbe;
	auto& cubeMap = app->_CubeMap;

	std::string path = "data/hdr/newport_loft.hdr";
	app->hdr_RectMap = load_hdr(path);

	cubeMap.insert({"environment", CubeMap{512, 512}});
	lightProbe->equirectangular_to_cubemap(app->hdr_RectMap, cubeMap["environment"]);
	cubeMap["environment"].generateMipmap();

	app->sh_volume.bake();
	app->sh_volume.relight();
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	app->sh_volume.project_sh();
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	app->sh_volume.print();


	cubeMap.insert({"irradiance", CubeMap{32, 32}});
	lightProbe->irradiance(cubeMap["irradiance"]);

	cubeMap.insert({"prefilter", CubeMap{256, 256, true}});
	lightProbe->prefilter(cubeMap["prefilter"]);

	app->brdfLUT.imagef(512, 512);
	Shaders::brdfShader.bind();
	app->brdfLUT.render_to(app->screen_quad);

}

App& App::get()
{
	assert(app);
	return *app;
}

void App::render()
{
	clear();
	render_imgui();
	render_3d();
}

CubeMap& App::EnvMap(std::string name)
{
	if (name.empty())
		name = map_choices[map_current];
	if(_CubeMap.find(name) == _CubeMap.end()){
		fmt::print("can not find cubemap {}\n", name);
	}
	return _CubeMap[name];
}

void App::clear()
{
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClearDepth(depth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void App::render_imgui()
{
	ImGui::Begin("PRT");
	// ImGui::Checkbox("Imgui Demo Window", &show_demo_window);
	// ImGui::Checkbox("tonemap", &tonemap);
	// ImGui::SameLine();
	// ImGui::Checkbox("gamma", &gamma);

	// ImGui::Separator();

	// ImGui::Checkbox("ray tracing", &ray_tracing);
	// if (ImGui::SliderInt("max path length", &max_path_length, 1, 10))
	// 	camera.dirty = true;
	// if (ImGui::Button("bake SH"))
	// 	bake_SH(model->meshes[0]);
	// ImGui::SliderInt("bake resolution", &sh_resolution, 10, 100);

	// ImGui::Separator();

	ImGui::Checkbox("render model", &render_model);
	ImGui::SameLine();
	ImGui::Checkbox("rotate model", &rotate_model);
	ImGui::Checkbox("metal", &metal);
	ImGui::Checkbox("diffuse", &diffuse);
	ImGui::SameLine();
	ImGui::Checkbox("sh", &sh);
	ImGui::Checkbox("specular", &specular);
	ImGui::SliderFloat("roughness", &roughness, 0, 1);
	ImGui::DragFloat3("metal F0", F0, 0.01, 0, 1);
	ImGui::DragFloat3("dielectric albedo", albedo, 0.01, 0, 1);

	// ImGui::Separator();

	// ImGui::Checkbox("rotate env", &rotate_env);
	// ImGui::SameLine();
	// ImGui::Checkbox("white_bk", &white_bk);
	// ImGui::SliderFloat("lod", &lod, 0, 4);
	// ImGui::ListBox("skybox", &map_current, map_choices.data(), map_choices.size());

	ImGui::Separator();
	ImGui::Checkbox("multi_bounce", &multi_bounce);
	ImGui::SameLine();
	ImGui::SliderFloat("atten", &atten, 0, 1);
	ImGui::Checkbox("render SH probe", &render_SH_probe);
	ImGui::DragFloat3("light position", cast_light_position, 0.01, -5, 5);
	ImGui::SliderFloat("intensity", &cast_light_intensity, 0, 300);
	ImGui::SliderFloat("cutoff", &cast_light_cut_off, 0, 1);


	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void App::render_3d()
{

	app->sh_volume.relight();
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	app->sh_volume.project_sh();
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	Mat_projection = glm::perspective(glm::radians(camera.Zoom), (float)plt.SCR_WIDTH / (float)plt.SCR_HEIGHT, 0.1f, 100.f);

	if (scene){
		static auto set_shader = [this]()
		{
			Shaders::castlightShader.bind();
			sh_volume.bind_sh_tex(Shaders::castlightShader);
			Shaders::castlightShader.uniform("view", camera.GetViewMatrix());
			Shaders::castlightShader.uniform("projection", Mat_projection);
			Shaders::castlightShader.uniform("light.intensity", cast_light_intensity*glm::vec3(1,1,1));
			Shaders::castlightShader.uniform("light.position", glm::vec3{cast_light_position[0], cast_light_position[1], cast_light_position[2]});
			Shaders::castlightShader.uniform("light.direction", glm::vec3(1,0,0));
			Shaders::castlightShader.uniform("light.cutOff", cast_light_cut_off);
			Shaders::castlightShader.uniform("ambient.position", glm::vec3(0,0,0));
			Shaders::castlightShader.uniform("ambient.intensity", 0.f*glm::vec3(1));
		};
		set_shader();
		scene->render(Shaders::castlightShader);
	}

	// if (skybox && rotate_env) //rotate env first, then render model
	// 	skybox->Mat_rotate = glm::rotate(skybox->Mat_rotate, (float)(1 * ImGui::GetIO().DeltaTime), glm::vec3(0.0f, 1.0f, 0.0f));
	// if (model && rotate_model)
	// 	model->Mat_model = glm::rotate(model->Mat_model, (float)ImGui::GetIO().DeltaTime, glm::vec3(0.5f, 1.0f, 0.0f));

	// if (skybox)
	// {
	// 	skybox->setShader();
	// 	skybox->render();
	// }
	static auto set_envshader = []()
	{
		auto &app = App::get();
		Shaders::envShader.bind();
		// Shaders::envShader.uniform("environment", app.EnvMap("environment").active(0));
		Shaders::envShader.uniform("irradiance", app.EnvMap("irradiance").active(1));
		Shaders::envShader.uniform("brdfLUT", app.brdfLUT.active(2));
		Shaders::envShader.uniform("prefilterMap", app.EnvMap("prefilter").active(3));
		app.sh_volume.bind_sh_tex(Shaders::envShader);

		Shaders::envShader.uniform("view", app.camera.GetViewMatrix());
		Shaders::envShader.uniform("projection", app.Mat_projection);
		Shaders::envShader.uniform("cameraPos", app.camera.Position);
		Shaders::envShader.uniform("sh", app.sh);
		Shaders::envShader.uniform("envRotate", glm::transpose(app.skybox->Mat_rotate));

		Shaders::envShader.uniform("tonemap", app.tonemap);
		Shaders::envShader.uniform("gamma", app.gamma);

		Shaders::envShader.uniform("metal", app.metal);
		Shaders::envShader.uniform("diffuse", app.diffuse);
		Shaders::envShader.uniform("specular", app.specular);
		Shaders::envShader.uniform("roughness", app.roughness);
		if (app.metal)
			Shaders::envShader.uniform("F0", glm::vec3{app.F0[0], app.F0[1], app.F0[2]});
		else
			Shaders::envShader.uniform("F0", glm::vec3{0.04});
		Shaders::envShader.uniform("albedo", glm::vec3{app.albedo[0], app.albedo[1], app.albedo[2]});
	};

	if (model && render_model)
	{
		set_envshader();
		model->render(Shaders::envShader);
	}

	if (probe_mesh && render_SH_probe)
	{
		set_envshader();
		for (auto pos : sh_volume.world_position) { //TODO: instance drawing results in [out of memory], why?
			probe_mesh->Mat_model = glm::translate(glm::mat4(1), pos);
			probe_mesh->render(Shaders::envShader);
		}

	}
}
