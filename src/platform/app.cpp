#include "app.h"
#include "platform.h"
#include "util/load.h"

App::App(Platform& plt)
	: plt(plt) {

	//    model = std::make_unique<Model>("data/buddha.obj");
	model = std::make_unique<Model>("data/cube.obj");
	skybox = std::make_unique<SkyBox>();
	lightProbe = std::make_unique<LightProbe>(*skybox);
}

void App::setup(Platform& plt)
{
	data = new App(plt);

	auto& lightProbe = data->lightProbe;
	auto& cubeMap = data->_CubeMap;

	//std::vector<std::string> faces
 //   {
 //       "data/skybox/right.jpg",
 //       "data/skybox/left.jpg",
 //       "data/skybox/top.jpg",
 //       "data/skybox/bottom.jpg",
 //       "data/skybox/front.jpg",
 //       "data/skybox/back.jpg"
 //   };
	//cubeMap.insert({ "environment", CubeMap{faces} });

	data->hdr_RectMap = load_hdr("data/hdr/newport_loft.hdr");
	cubeMap.insert({"environment", CubeMap{512, 512}});
	lightProbe->equirectangular_to_cubemap(cubeMap["environment"]);
	cubeMap["environment"].generateMipmap();
	cubeMap.insert({"irradiance", CubeMap{32, 32}});
	lightProbe->irradiance(cubeMap["irradiance"]);
	cubeMap.insert({"prefilter", CubeMap{256, 256, true}});
	lightProbe->prefilter(cubeMap["prefilter"]);

}

App& App::get()
{
	assert(data);
	return *data;
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
	ImGui::Checkbox("Imgui Demo Window", &show_demo_window);
	ImGui::Checkbox("render model", &render_model);
	ImGui::Checkbox("rotate", &rotate);
	ImGui::Checkbox("tonemap", &tonemap);
	ImGui::Checkbox("gamma", &gamma);

	ImGui::Separator();
	ImGui::Checkbox("metal", &metal);
	ImGui::DragFloat3("metal F0", F0, 0.01, 0, 1);
	ImGui::DragFloat3("dielectric albedo", albedo, 0.01, 0, 1);
	ImGui::Separator();


	ImGui::DragFloat("lod", &lod, 0.01, 0, 4);
    ImGui::ListBox("skybox", &map_current, map_choices.data(), map_choices.size());

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void App::render_3d()
{

	Mat_projection = glm::perspective(glm::radians(camera.Zoom), (float)plt.SCR_WIDTH / (float)plt.SCR_HEIGHT, 0.1f, 100.f);

	if (model && render_model) {
		if (rotate)
			model->Mat_model = glm::rotate(model->Mat_model, (float)ImGui::GetIO().DeltaTime, glm::vec3(0.5f, 1.0f, 0.0f));
		model->render();
	}
	if (skybox) {
		skybox->setShader();
		skybox->render();
	}
}
