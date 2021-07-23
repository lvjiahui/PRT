#include "volume.h"
#include "SH_function.h"
#include "platform/app.h"
#include "util/util.h"
#include <iostream>
#include <fstream>
#include <map>
#include <random>
#include <array>
// #include "cgal/light_probe.h"
#include "raytracing/light_probe.h"


void SH_volume::init(float target_probe_space, float target_volume_space, glm::vec3 _scene_size)
{
	//actual space is smaller than target space
	volume_res = glm::ceil(2.f * _scene_size / target_volume_space);
	probe_res = glm::ceil(2.f * _scene_size / target_probe_space);
	fmt::print("probe res: {}x{}x{}\n", probe_res.x, probe_res.y, probe_res.z);
	fmt::print("volume res: {}x{}x{}\n", volume_res.x, volume_res.y, volume_res.z);
	scene_size = _scene_size;
	// sh volume
	glGenTextures(num_sh_tex, sh_tex);
	glGenTextures(num_sh_tex, volume_tex);
	for (int i = 0; i < num_sh_tex; i++) {
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, probe_res.x, probe_res.y, probe_res.z);

		glBindTexture(GL_TEXTURE_3D, volume_tex[i]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		// glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, volume_res.x, volume_res.y, volume_res.z);
	}

	glGenTextures(1, &volume_weight0123);
	glGenTextures(1, &volume_weight4567);
	glBindTexture(GL_TEXTURE_3D, volume_weight0123);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, volume_res.x, volume_res.y, volume_res.z);
	glBindTexture(GL_TEXTURE_3D, volume_weight4567);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, volume_res.x, volume_res.y, volume_res.z);

	// probe cubemap
	glGenTextures(1, &GBuffer_norm);
	glGenTextures(1, &GBuffer_pos);
	glGenTextures(1, &GBuffer_ID);

	glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_norm);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA16F, cubemap_res, cubemap_res);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_pos);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA16F, cubemap_res, cubemap_res);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_ID);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_R32F, cubemap_res, cubemap_res); //GL_R32I does not work, why??????
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//should be consistent with SH.glsl
	glm::vec3 ds = 2.f / glm::vec3(probe_res) * scene_size;
	for (int z_i = 0; z_i < probe_res.z; z_i++)
		for (int y_i = 0; y_i < probe_res.y; y_i++)
			for (int x_i = 0; x_i < probe_res.x; x_i++)
			{
				glm::vec3 pos = -scene_size + ds * (glm::vec3(0.5) + glm::vec3(x_i, y_i ,z_i));
				probe_positions.push_back(pos);
			}

	

	// precomputed SH and solid angle
	glGenBuffers(1, &transfer_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, transfer_buffer);
	glGenTextures(1, &transfer_tex);
	glBindTexture(GL_TEXTURE_BUFFER, transfer_tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, transfer_buffer);

	glGenBuffers(1, &ID_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, ID_buffer);
	glGenTextures(1, &ID_tex);
	glBindTexture(GL_TEXTURE_BUFFER, ID_tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, ID_buffer);

	glGenBuffers(1, &rad_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, rad_buffer);
	glGenTextures(1, &rad_tex);
	glBindTexture(GL_TEXTURE_BUFFER, rad_tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, rad_buffer);

	glGenBuffers(1, &primitive_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, primitive_buffer);
	glGenTextures(1, &primitive_tex);
	glBindTexture(GL_TEXTURE_BUFFER, primitive_tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, primitive_buffer);

	glGenTextures(1, &probe_range);
	glBindTexture(GL_TEXTURE_3D, probe_range);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RG32UI, probe_res.x, probe_res.y, probe_res.z);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void SH_volume::set_volume_filter(int FILTER)
{
	for (int i = 0; i < num_sh_tex; i++) {
		glBindTexture(GL_TEXTURE_3D, volume_tex[i]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, FILTER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, FILTER);
	}
}

SH_volume::~SH_volume()
{
	glDeleteTextures(num_sh_tex, sh_tex);
	//TODO: release
}

void SH_volume::bake()
{
	// precomputed SH and solid angle
	precompute();
	set_visibility();
}


void SH_volume::precompute(){
	// glEnable(GL_CULL_FACE);

	auto capture_GBuffer = [this](glm::vec3 pos){
		GLint m_viewport[4];
		glGetIntegerv(GL_VIEWPORT, m_viewport);
		glViewport(0, 0, cubemap_res, cubemap_res);
		auto& app = App::get();
		gbuffer_shader.bind();
		gbuffer_shader.uniform("model", glm::mat4(1));
		gbuffer_shader.uniform("projection", captureProjection);

		app.captureFB.bind(); //glBindFramebuffer
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemap_res, cubemap_res);


		// fmt::print("probe_pos: {} {} {}\n", pos[0], pos[1], pos[2]);
		for (int face = 0; face < 6; face++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, GBuffer_pos, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, GBuffer_norm, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, GBuffer_ID, 0);
			GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    		glDrawBuffers(3, attachments);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    			std::cout << "Framebuffer not complete!" << std::endl;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			gbuffer_shader.uniform("view", captureViews(pos, face));
			app.scene->render(gbuffer_shader);
		}
	

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
	};

	enum class Dir{
		POSITIVE_X,
		NEGATIVE_X,
		POSITIVE_Y,
		NEGATIVE_Y,
		POSITIVE_Z,
		NEGATIVE_Z,
	};

	std::vector<SH9> precomputed_transfer;
	std::vector<GLuint> precomputed_ID;
	struct Point{
		glm::vec3 pos;
		glm::vec3 norm;
	};
	using Cluster_ID = std::array<int, 4>; //x,y,z,direction
	std::map<Cluster_ID, GLuint> surfel_ID; //serialized ID
	std::map<GLuint, std::vector<Point>> surfel_cluster;
	std::vector<glm::uvec2> range;
	num_primitive = 0;
	auto get_surfel_ID = [&surfel_ID, this](glm::vec3 pos, glm::vec3 norm)
	{
		int direction = 0;
		if(std::abs(norm.x) > std::abs(norm.y) &&  std::abs(norm.x) > std::abs(norm.z)){
			direction = norm.x > 0 ? static_cast<int>(Dir::POSITIVE_X) : static_cast<int>(Dir::NEGATIVE_X);
		}
		if(std::abs(norm.y) > std::abs(norm.x) &&  std::abs(norm.y) > std::abs(norm.z)){
			direction = norm.y > 0 ? static_cast<int>(Dir::POSITIVE_Y) : static_cast<int>(Dir::NEGATIVE_Y);
		}
		if(std::abs(norm.z) > std::abs(norm.x) &&  std::abs(norm.z) > std::abs(norm.y)){
			direction = norm.z > 0 ? static_cast<int>(Dir::POSITIVE_Z) : static_cast<int>(Dir::NEGATIVE_Z);
		}
		pos = glm::floor(1.f * pos);
		Cluster_ID cluster_ID{static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z), direction};
		if (surfel_ID.find(cluster_ID) == surfel_ID.end()){
			surfel_ID[cluster_ID] = num_primitive++;
		}
		return surfel_ID[cluster_ID];
	};

	fmt::print("Baking ..........\n");


	int num_probe = probe_res.x * probe_res.y * probe_res.z;
	for (int i = 0; i < num_probe; i++) {
		glm::vec3 probe_pos = probe_positions[i];
		capture_GBuffer(probe_pos);
		float sum = 0;
		std::map<GLuint, SH9> transfer_weight;
		for (int face = 0; face < 6; face++)
		{
			// std::vector<GLfloat> ID_cpu(cubemap_res * cubemap_res, 0);
			// glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_ID);
			// glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, GL_RED, GL_FLOAT, ID_cpu.data());
			std::vector<glm::vec3> pos_cpu(cubemap_res * cubemap_res, glm::vec3(0, 0, 0));
			glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_pos);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, GL_RGB, GL_FLOAT, pos_cpu.data());
			std::vector<glm::vec3> normal_cpu(cubemap_res * cubemap_res, glm::vec3(0, 0, 0));
			glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_norm);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, GL_FLOAT, normal_cpu.data());
			for (int tex = 0; tex < pos_cpu.size(); tex++) {
				if (glm::length(normal_cpu[tex]) < 0.1)
					continue; //TODO: sky visibility?
				glm::vec3 tex_pos = pos_cpu[tex] - probe_pos;
				if (glm::dot(tex_pos, normal_cpu[tex]) > 0)
					continue; //TODO: back face
				glm::vec3 cube_coord = tex_pos / std::max({std::abs(tex_pos.x), std::abs(tex_pos.y), std::abs(tex_pos.z)});
				float weight = glm::dot(cube_coord, cube_coord);
				weight *= std::sqrt(weight);
				float solid_angle = 4.0 / cubemap_res / cubemap_res / weight;
				glm::vec3 d = glm::vec3(cube_coord.z, cube_coord.x, cube_coord.y); // to directX
				d = glm::normalize(d);
				// GLuint ID = static_cast<GLuint>(ID_cpu[tex]);
				GLuint ID = get_surfel_ID(pos_cpu[tex], normal_cpu[tex]);
				surfel_cluster[ID].push_back(Point{pos_cpu[tex], normal_cpu[tex]});
				transfer_weight[ID] += SH9{d} * solid_angle;
				sum += solid_angle;
			}
		}
		// fmt::print("solid angle {} PI\n", sum/PI);
		int start = precomputed_transfer.size();
		for(auto [ID, SH] : transfer_weight){
			precomputed_ID.push_back(ID);
			precomputed_transfer.push_back(SH);
		}
		int end = precomputed_transfer.size();
		range.push_back(glm::ivec2(start, end));
	}
	glBindBuffer(GL_TEXTURE_BUFFER, transfer_buffer);
	glBufferData(GL_TEXTURE_BUFFER, precomputed_transfer.size() * sizeof(SH9), precomputed_transfer.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, ID_buffer);
	glBufferData(GL_TEXTURE_BUFFER, precomputed_ID.size() * sizeof(GLuint), precomputed_ID.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, rad_buffer);
	glBufferData(GL_TEXTURE_BUFFER, num_primitive * sizeof(glm::vec4), nullptr, GL_DYNAMIC_COPY);

	glBindTexture(GL_TEXTURE_3D, probe_range);
	// glTexSubImage3D(GL_TEXTURE_3D, 0, 
	// 	0, 0, 0,
	// 	probe_res, probe_res, probe_res, 
	// 	GL_RG_INTEGER, GL_UNSIGNED_INT, range.data());
	int offset = 0;
	for (int z_i = 0; z_i < probe_res.z; z_i++)
		for (int y_i = 0; y_i < probe_res.y; y_i++)
			for (int x_i = 0; x_i < probe_res.x; x_i++)
			{
				glTexSubImage3D(GL_TEXTURE_3D, 0, 
					x_i, y_i, z_i,
					1, 1, 1, 
					GL_RG_INTEGER, GL_UNSIGNED_INT, range.data() + offset);
				offset++;
			}

	// average of cluster
	fmt::print("num_primitive {} \n", num_primitive);
	std::ofstream outfile{"data/sample.txt"};
	std::vector<glm::vec3> primitive_data(2*num_primitive);
	for (auto [ID, points] : surfel_cluster){
		glm::vec3 aver_pos{0}, aver_norm{0};
		for (auto [pos, norm] : points){
			aver_pos += pos;
			aver_norm += norm;
		}
		aver_pos = 1.f / points.size() * aver_pos;
		aver_norm = glm::normalize(1.f / points.size() * aver_norm);
		outfile <<  aver_pos.x << " " << aver_pos.y << " " << aver_pos.z << " " << aver_norm.x << " " << aver_norm.y << " " << aver_norm.z << "\n";
		primitive_data[2*ID + 0] = aver_pos;
		primitive_data[2*ID + 1] = aver_norm;
	}
	outfile.close();
	glBindBuffer(GL_TEXTURE_BUFFER, primitive_buffer);
	glBufferData(GL_TEXTURE_BUFFER, num_primitive * 2 * sizeof(glm::vec3), primitive_data.data(), GL_STATIC_DRAW);
}

void SH_volume::set_visibility(){
	fmt::print("computing visibility ..........\n");
	auto weight = calculate_weight(*App::get().scene, probe_res, volume_res, scene_size);

	glBindTexture(GL_TEXTURE_3D, volume_weight0123);
	glTexSubImage3D(GL_TEXTURE_3D, 0, 
		0, 0, 0,
		volume_res.x, volume_res.y, volume_res.z, 
		GL_RGBA, GL_FLOAT, weight.weight0123.data());

	glBindTexture(GL_TEXTURE_3D, volume_weight4567);
	glTexSubImage3D(GL_TEXTURE_3D, 0, 
		0, 0, 0,
		volume_res.x, volume_res.y, volume_res.z, 
		GL_RGBA, GL_FLOAT, weight.weight4567.data());
}

void SH_volume::print()
{
	int num_probe = probe_res.x * probe_res.y * probe_res.z;


	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	for (int sh_i = 0; sh_i < num_sh_tex; sh_i++) {
		std::vector<glm::vec4> sh_cpu(num_probe, glm::vec4(0, 0, 0, 0));
		glBindTexture(GL_TEXTURE_3D, sh_tex[sh_i]);
		glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, sh_cpu.data());
		for (int p_i = 0; p_i < num_probe; p_i++) {
			fmt::print("SH_{} {} {} {} {}\n", sh_i, sh_cpu[p_i][0], sh_cpu[p_i][1], sh_cpu[p_i][2], sh_cpu[p_i][3]);
		}
	}

}

inline float random() {
	static std::uniform_real_distribution<float> distribution(0.0, 1.0);
	static std::mt19937 generator;
	return distribution(generator);
}
void SH_volume::relight()
{
	int num_probe = probe_res.x * probe_res.y * probe_res.z;
	auto& app = App::get();
	relight_shader.bind();
	relight_shader.uniform("light.intensity", app.cast_light_intensity*glm::vec3(1,1,1));
	relight_shader.uniform("light.position", glm::vec3{app.cast_light_position[0], app.cast_light_position[1], app.cast_light_position[2]});
	relight_shader.uniform("light.direction", glm::vec3(1,0,0));
	relight_shader.uniform("light.cutOff", app.cast_light_cut_off);
	relight_shader.uniform("ambient.position", glm::vec3(0,0,0));
	relight_shader.uniform("ambient.intensity", 0.f*glm::vec3(1));
	relight_shader.uniform("multi_bounce", app.multi_bounce);
	relight_shader.uniform("atten", app.atten);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app.sky_shadow.depthMap);
	relight_shader.uniform("shadowMap", 0);
	relight_shader.uniform("lightSpaceMatrix", app.sky_shadow.lightSpaceMatrix);
	relight_shader.uniform("sky.intensity", glm::vec3(app.sky_intensity[0], app.sky_intensity[1], app.sky_intensity[2]));
	relight_shader.uniform("sky.direction", app.sky_shadow.direction);
	bind_volume_tex(relight_shader);
	relight_shader.uniform("sh_shift", app.sh_shift);
	glBindImageTexture(0, rad_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_BUFFER, primitive_tex);
	relight_shader.uniform("primitive_vert", 1);
	relight_shader.uniform("num_primitive", num_primitive);
	// relight_shader.uniform("seed_u", random());
	// relight_shader.uniform("seed_v", random());
	glDispatchCompute((num_primitive+127)/128, 1, 1);
}

void SH_volume::project_sh()
{

	project_shader.bind();
	for (int i = 0; i < num_sh_tex; i++) {
		glBindImageTexture(i, sh_tex[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	}
	int texture_unit = 0;
	glActiveTexture(GL_TEXTURE0+texture_unit);
	glBindTexture(GL_TEXTURE_BUFFER, rad_tex);
	project_shader.uniform("radiance", texture_unit);
	texture_unit++;

	glActiveTexture(GL_TEXTURE0+texture_unit);
	glBindTexture(GL_TEXTURE_BUFFER, transfer_tex);
	project_shader.uniform("transfer", texture_unit);
	texture_unit++;
	
	glActiveTexture(GL_TEXTURE0+texture_unit);
	glBindTexture(GL_TEXTURE_3D, probe_range);
	project_shader.uniform("probe_range", texture_unit);
	texture_unit++;

	glActiveTexture(GL_TEXTURE0+texture_unit);
	glBindTexture(GL_TEXTURE_BUFFER, ID_tex);
	project_shader.uniform("range_ID", texture_unit);
	texture_unit++;

	glDispatchCompute(probe_res.x, probe_res.y, probe_res.z);


	//transfer2volume
	transfer2volume_shader.bind();
	for (int i = 0; i < num_sh_tex; i++) {
		glBindImageTexture(i, volume_tex[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	}
	std::string sh_tex_name[7] = {
		"SH_Ar",
		"SH_Ag",
		"SH_Ab",
		"SH_Br",
		"SH_Bg",
		"SH_Bb",
		"SH_C"};
	for (int i = 0; i < num_sh_tex; i++) {
		int tex_unit = 79 - num_sh_tex + i;
		glActiveTexture(GL_TEXTURE0 + tex_unit);
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
		transfer2volume_shader.uniform(sh_tex_name[i], tex_unit);
	}
	texture_unit = 0;
	glActiveTexture(GL_TEXTURE0+texture_unit);
	glBindTexture(GL_TEXTURE_3D, volume_weight0123);
	transfer2volume_shader.uniform("volume_weight0123", texture_unit);
	texture_unit++;
	glActiveTexture(GL_TEXTURE0+texture_unit);
	glBindTexture(GL_TEXTURE_3D, volume_weight4567);
	transfer2volume_shader.uniform("volume_weight4567", texture_unit);
	texture_unit++;

	transfer2volume_shader.uniform("volume_res", volume_res);
	transfer2volume_shader.uniform("probe_res", probe_res);
	// transfer2volume_shader.uniform("scene_size", scene_size);
	glDispatchCompute((volume_res.x+7)/8, (volume_res.y+7)/8, (volume_res.z+7)/8);
}

void SH_volume::bind_sh_tex(Shader& shader)
{
	shader.bind();
	shader.uniform("scene_size", scene_size);
	for (int i = 0; i < num_sh_tex; i++) {
		int tex_unit = 79 - num_sh_tex + i;
		glActiveTexture(GL_TEXTURE0 + tex_unit);
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
		shader.uniform("SH_volume" + std::to_string(i), tex_unit);
	}
}

void SH_volume::bind_volume_tex(Shader& shader)
{
	// bind_sh_tex(shader);
	// return;

	shader.bind();
	shader.uniform("scene_size", scene_size);
	for (int i = 0; i < num_sh_tex; i++) {
		int tex_unit = 79 - num_sh_tex + i;
		glActiveTexture(GL_TEXTURE0 + tex_unit);
		glBindTexture(GL_TEXTURE_3D, volume_tex[i]);
		shader.uniform("SH_volume" + std::to_string(i), tex_unit);
	}
}

glm::mat4 SH_volume::captureViews(glm::vec3 position, int face){
    static glm::vec3 fronts[6] =
    {
        glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(-1.0f,  0.0f,  0.0f),
        glm::vec3(0.0f,  1.0f,  0.0f),
        glm::vec3(0.0f, -1.0f,  0.0f),
        glm::vec3(0.0f,  0.0f,  1.0f),
        glm::vec3(0.0f,  0.0f, -1.0f),
    };
	static glm::vec3 ups[6] =
	{
		glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(0.0f,  0.0f,  1.0f),
		glm::vec3(0.0f,  0.0f, -1.0f),
		glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(0.0f, -1.0f,  0.0f)
	};
    return glm::lookAt(position, position + fronts[face], ups[face]);

}

