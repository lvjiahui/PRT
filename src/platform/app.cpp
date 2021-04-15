#include "app.h"
#include "platform.h"
#include "util/util.h"
#include "sh/image.h"
#include "sh/spherical_harmonics.h"
#include "raytracing/raytracing.h"

App::App(Platform& plt)
	: plt(plt) {
		pixels.resize(plt.SCR_WIDTH * plt.SCR_HEIGHT);
		pixels_w.resize(plt.SCR_WIDTH * plt.SCR_HEIGHT);
		model = std::make_unique<Model>("data/buddha.obj");
		//  model = std::make_unique<Model>("data/sphere.obj");
		skybox = std::make_unique<SkyBox>();
		lightProbe = std::make_unique<LightProbe>(*skybox);
		rtscene = std::make_unique<RTScene>(model->meshes[0]);
}

void App::setup(Platform& plt)
{
	app = new App(plt);

    Shaders::brdfShader = Shader{ fs::path{"src/opengl/brdf.vert"}, fs::path{"src/opengl/brdf.frag"} };
    Shaders::screenShader = Shader{ fs::path{"src/opengl/screen.vert"}, fs::path{"src/opengl/screen.frag"} };

	auto& lightProbe = app->lightProbe;
	auto& cubeMap = app->_CubeMap;

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

	auto hdr = sh::HDR_Image{"data/hdr/newport_loft.hdr"};
	 // auto hdr = sh::HDR_Image{"data/hdr/Gloucester-Church_Ref.hdr"};
	// hdr.SetAll([&](double phi, double theta) {
	// 	auto cos = sh::ToVector(phi, theta).dot(Eigen::Vector3d::UnitY());
	// 	if(cos < 0) cos = 0;
	// 	cos = cos*cos*cos;
	// 	return Eigen::Array3f{cos};
	// });
	app->hdr_RectMap.imagef(hdr.width(), hdr.height(), hdr.pixels_);

	int order = 2;
	auto sh_coeffs = sh::ProjectEnvironment_Par(order, hdr);
	app->env_sh.clear();
	for(auto coeff : *sh_coeffs) app->env_sh.push_back(glm::vec3{coeff.x(),coeff.y(),coeff.z()});
	hdr.SetAll([&](double phi, double theta) {
		Eigen::Vector3d normal = sh::ToVector(phi, theta);
		Eigen::Array3f irradiance = sh::RenderDiffuseIrradiance(*sh_coeffs, normal);
		return irradiance;
		// return sh::EvalSHSum(order, *sh_coeffs, phi, theta);
	});

	Tex2D sh_env{};
	sh_env.imagef(hdr.width(), hdr.height(), hdr.pixels_);
	cubeMap.insert({"sh_env", CubeMap{512, 512}});
	lightProbe->equirectangular_to_cubemap(sh_env, cubeMap["sh_env"]);

	cubeMap.insert({"environment", CubeMap{512, 512}});
	lightProbe->equirectangular_to_cubemap(app->hdr_RectMap, cubeMap["environment"]);
	cubeMap["environment"].generateMipmap();

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
	ImGui::Checkbox("Imgui Demo Window", &show_demo_window);
	ImGui::Checkbox("tonemap", &tonemap);
	ImGui::SameLine();
	ImGui::Checkbox("gamma", &gamma);

	ImGui::Separator();

	ImGui::Checkbox("ray tracing", &ray_tracing);
	if(ImGui::SliderInt("max path length", &max_path_length, 1, 10))
		camera.dirty = true;
	if(ImGui::Button("bake SH"))
		bake_SH(model->meshes[0]);
	ImGui::SliderInt("bake resolution", &sh_resolution, 10, 100);

	ImGui::Separator();

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
	
	ImGui::Separator();


	ImGui::Checkbox("rotate env", &rotate_env);
	ImGui::SameLine();
	ImGui::Checkbox("white_bk", &white_bk);
	ImGui::SliderFloat("lod", &lod, 0, 4);
    ImGui::ListBox("skybox", &map_current, map_choices.data(), map_choices.size());

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void App::render_3d()
{
	if(ray_tracing && rtscene){
		raytrace(*rtscene);
    	/* draw text to screen */
		//screenTex.imagef(plt.SCR_WIDTH, plt.SCR_HEIGHT, (float*)pixels.data());
		screenTex.imagei(plt.SCR_WIDTH, plt.SCR_HEIGHT, (unsigned char *)pixels.data());
		Shaders::screenShader.bind();
		Shaders::screenShader.uniform("screenTexture", screenTex.active());
		screen_quad.render();
		return;
	}

	Mat_projection = glm::perspective(glm::radians(camera.Zoom), (float)plt.SCR_WIDTH / (float)plt.SCR_HEIGHT, 0.1f, 100.f);
		
	if (skybox && rotate_env) //rotate env first, then render model
		skybox->Mat_rotate = glm::rotate(skybox->Mat_rotate, (float)(1 * ImGui::GetIO().DeltaTime), glm::vec3(0.0f, 1.0f, 0.0f));
	if (model && rotate_model)
		model->Mat_model = glm::rotate(model->Mat_model, (float)ImGui::GetIO().DeltaTime, glm::vec3(0.5f, 1.0f, 0.0f));
	
	if (skybox) {
		skybox->setShader();
		skybox->render();
	}
	if (model && render_model) {
		model->render();
	}
}
