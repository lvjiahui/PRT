#include "app.h"
#include "platform.h"
#include "util/load.h"

App::App(Platform& plt)
	: plt(plt) {

	model = std::make_unique<Model>("data/buddha.obj");

	std::vector<std::string> faces
    {
        "data/skybox/right.jpg",
        "data/skybox/left.jpg",
        "data/skybox/top.jpg",
        "data/skybox/bottom.jpg",
        "data/skybox/front.jpg",
        "data/skybox/back.jpg"
    };
	envTexture = loadCubemap(faces);

	sky_box = std::make_unique<SkyBox>();
}

void App::setup(Platform& plt)
{
	data = new App(plt);
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
	ImGui::Checkbox("rotate", &rotate);
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void App::render_3d()
{

	Mat_projection = glm::perspective(glm::radians(camera.Zoom), (float)plt.SCR_WIDTH / (float)plt.SCR_HEIGHT, 0.1f, 100.f);

	if (model) {
		if (rotate)
			model->Mat_model = glm::rotate(model->Mat_model, (float)ImGui::GetIO().DeltaTime, glm::vec3(0.5f, 1.0f, 0.0f));
		model->render();
	}
	if (sky_box) sky_box->render();
}
