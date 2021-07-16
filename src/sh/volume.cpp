#include "volume.h"
#include "SH_function.h"
#include "platform/app.h"
#include "util/util.h"
#include <iostream>
#include <fstream>
#include <map>
#include <random>

SH_volume::SH_volume(GLsizei _probe_res) : probe_res(_probe_res)
{

	// sh volume
	glGenTextures(num_sh_tex, sh_tex);
	for (int i = 0; i < num_sh_tex; i++) {
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, probe_res, probe_res, probe_res);
	}

	// probe 

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

	
	// float ds = 2*scene_size / (probe_res-1);
	float ds = 2*scene_size / probe_res;
	for (int z_i = 0; z_i < probe_res; z_i++)
		for (int y_i = 0; y_i < probe_res; y_i++)
			for (int x_i = 0; x_i < probe_res; x_i++)
			{
				// if(probe_res == 1) {
				// 	pos = glm::vec3(0,0,0);
				// } else {
				// 	pos = -scene_size + ds* glm::vec3(x_i, y_i ,z_i);
				// }
				glm::vec3 pos = -scene_size + ds * (glm::vec3(0.5) + glm::vec3(x_i, y_i ,z_i));
				world_position.push_back(pos);
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
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RG32UI, probe_res, probe_res, probe_res);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

}


void SH_volume::precompute(){

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

	std::vector<SH9> precomputed_transfer;
	std::vector<GLuint> precomputed_ID;
	std::vector<glm::uvec2> range;
	num_primitive = 0;

	fmt::print("Baking ..........\n");


	int num_probe = probe_res * probe_res * probe_res;
	for (int i = 0; i < num_probe; i++) {
		glm::vec3 probe_pos = world_position[i];
		capture_GBuffer(probe_pos);
		float sum = 0;
		std::map<GLuint, SH9> transfer_weight;
		for (int face = 0; face < 6; face++)
		{
			std::vector<GLfloat> ID_cpu(cubemap_res * cubemap_res, 0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_ID);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, GL_RED, GL_FLOAT, ID_cpu.data());
			std::vector<glm::vec3> pos_cpu(cubemap_res * cubemap_res, glm::vec3(0, 0, 0));
			glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_pos);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, GL_RGB, GL_FLOAT, pos_cpu.data());
			std::vector<glm::vec3> normal_cpu(cubemap_res * cubemap_res, glm::vec3(0, 0, 0));
			glBindTexture(GL_TEXTURE_CUBE_MAP, GBuffer_norm);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, GL_FLOAT, normal_cpu.data());
			for (int tex = 0; tex < ID_cpu.size(); tex++) {
				if (glm::length(normal_cpu[tex]) < 0.1)
					continue; //TODO: sky visibility?
				glm::vec3 tex_pos = pos_cpu[tex] - probe_pos;
				glm::vec3 cube_coord = tex_pos / std::max({std::abs(tex_pos.x), std::abs(tex_pos.y), std::abs(tex_pos.z)});
				float weight = glm::dot(cube_coord, cube_coord);
				weight *= std::sqrt(weight);
				float solid_angle = 4.0 / cubemap_res / cubemap_res / weight;
				glm::vec3 d = glm::vec3(cube_coord.z, cube_coord.x, cube_coord.y); // to directX
				d = glm::normalize(d);
				GLuint ID = static_cast<GLuint>(ID_cpu[tex]);
				if (ID >= num_primitive) num_primitive = ID+1;
				transfer_weight[ID] += SH9{d} * solid_angle;
				sum += solid_angle;
			}
		}
		// fmt::print("solid angle {} PI\n", sum/PI);
		int start = precomputed_transfer.size();
		for(auto [ID, SH] : transfer_weight){
			precomputed_transfer.push_back(SH);
			precomputed_ID.push_back(ID);
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
	for (int z_i = 0; z_i < probe_res; z_i++)
		for (int y_i = 0; y_i < probe_res; y_i++)
			for (int x_i = 0; x_i < probe_res; x_i++)
			{
				glTexSubImage3D(GL_TEXTURE_3D, 0, 
				x_i, y_i, z_i,
				1, 1, 1, 
				GL_RG_INTEGER, GL_UNSIGNED_INT, range.data() + probe_res * (probe_res * z_i + y_i) + x_i);
			}
	auto& app = App::get();
	const auto& index = app.scene->meshes[0].indices();
	const auto& vert = app.scene->meshes[0].verts();
	num_primitive = index.size() / 3;
	fmt::print("num_primitive {} \n", num_primitive);
	std::vector<glm::vec3> primitive_vert;
	for(int i = 0; i < num_primitive; i++){
		primitive_vert.push_back(vert[index[3*i + 0]].pos);
		primitive_vert.push_back(vert[index[3*i + 1]].pos);
		primitive_vert.push_back(vert[index[3*i + 2]].pos);
	}
	glBindBuffer(GL_TEXTURE_BUFFER, primitive_buffer);
	glBufferData(GL_TEXTURE_BUFFER, num_primitive * 3 * sizeof(glm::vec3), primitive_vert.data(), GL_STATIC_DRAW);
}

void SH_volume::print()
{
	int num_probe = probe_res * probe_res * probe_res;


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
	int num_probe = probe_res * probe_res * probe_res;
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
	relight_shader.uniform("sky.intensity", glm::vec3(app.sky_intensity));
	relight_shader.uniform("sky.direction", app.sky_shadow.direction);
	bind_sh_tex(relight_shader);
	glBindImageTexture(0, rad_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_BUFFER, primitive_tex);
	relight_shader.uniform("primitive_vert", 1);
	relight_shader.uniform("num_primitive", num_primitive);
	relight_shader.uniform("seed_u", random());
	relight_shader.uniform("seed_v", random());
	glDispatchCompute((num_primitive+127)/128, 1, 1);
}

void SH_volume::project_sh()
{

	// relight_project_shader.bind();
	// relight_project_shader.uniform("probe_res", probe_res);
	// // direct light
	// auto& app = App::get();
	// relight_project_shader.uniform("light.intensity", app.cast_light_intensity*glm::vec3(1,1,1));
	// relight_project_shader.uniform("light.position", glm::vec3{app.cast_light_position[0], app.cast_light_position[1], app.cast_light_position[2]});
	// relight_project_shader.uniform("light.direction", glm::vec3(1,0,0));
	// relight_project_shader.uniform("light.cutOff", app.cast_light_cut_off);
	// relight_project_shader.uniform("ambient.position", glm::vec3(0,0,0));
	// relight_project_shader.uniform("ambient.intensity", 0.f*glm::vec3(1));
	// relight_project_shader.uniform("multi_bounce", app.multi_bounce);
	// relight_project_shader.uniform("atten", app.atten);
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, app.sky_shadow.depthMap);
	// relight_project_shader.uniform("shadowMap", 0);
	// relight_project_shader.uniform("lightSpaceMatrix", app.sky_shadow.lightSpaceMatrix);
	// relight_project_shader.uniform("sky.intensity", glm::vec3(app.sky_intensity));
	// relight_project_shader.uniform("sky.direction", app.sky_shadow.direction);
	// //multi bounce
	// bind_sh_tex(relight_project_shader);
	// // SH project
	// for (int i = 0; i < num_sh_tex; i++) {
	// 	glBindImageTexture(i, sh_tex[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	// }
	// glActiveTexture(GL_TEXTURE0 + 74);
	// glBindTexture(GL_TEXTURE_2D, GB_pos_2D);
	// relight_project_shader.uniform("WorldPos", 74);
	// glActiveTexture(GL_TEXTURE0 + 75);
	// glBindTexture(GL_TEXTURE_2D, GB_norm_2D);
	// relight_project_shader.uniform("Normal", 75);

	// glDispatchCompute(8, 64, 1);

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

	glDispatchCompute(probe_res/4, probe_res/4, probe_res/4);
}

void SH_volume::bind_sh_tex(Shader& shader)
{
	shader.bind();
	for (int i = 0; i < num_sh_tex; i++) {
		int tex_unit = 16 - num_sh_tex + i;
		glActiveTexture(GL_TEXTURE0 + tex_unit);
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
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

