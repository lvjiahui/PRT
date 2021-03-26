#include "app.h"
#include "platform.h"

App::App(Platform& plt)
	: plt(plt) {

	//mesh_shader = Shader{ Shaders::mesh_v, Shaders::mesh_f };
	mesh_shader = Shader{ fs::path{"src/shader/mesh.vs"}, fs::path{"src/shader/mesh.fs"} };
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
	sky_box = std::make_unique<SkyBox>(faces);
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

	glm::mat4 M_model(1); //model矩阵，局部坐标变换至世界坐标
	if (rotate)
		M_model = glm::rotate(M_model, (float)ImGui::GetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
	glm::mat4 M_view = camera.GetViewMatrix(); //view矩阵，世界坐标变换至观察坐标系
	M_projection = glm::perspective(glm::radians(camera.Zoom), (float)plt.SCR_WIDTH / (float)plt.SCR_HEIGHT, 0.1f, 100.f);


	mesh_shader.bind();
	// 向着色器中传入参数
	mesh_shader.uniform("model", M_model);
	mesh_shader.uniform("view", M_view);
	mesh_shader.uniform("projection", M_projection);
	mesh_shader.uniform("objectColor", glm::vec3{ .8f, .8f , .8f });

	if (model) model->render(mesh_shader);
	if (sky_box) sky_box->render();
}
