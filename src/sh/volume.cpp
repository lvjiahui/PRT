#include "volume.h"
#include "platform/app.h"
#include "util/util.h"
#include <iostream>


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
	int num_probe = probe_res * probe_res * probe_res;
	probe_radiance.resize(num_probe);
	probe_GB_norm.resize(num_probe);
	probe_GB_pos.resize(num_probe);
	glGenTextures(num_probe, probe_radiance.data());
	glGenTextures(num_probe, probe_GB_norm.data());
	glGenTextures(num_probe, probe_GB_pos.data());
	for (int i = 0; i < num_probe; i++) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, probe_radiance[i]);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA16F, cubemap_res, cubemap_res);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_CUBE_MAP, probe_GB_norm[i]);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA16F, cubemap_res, cubemap_res);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_CUBE_MAP, probe_GB_pos[i]);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA16F, cubemap_res, cubemap_res);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	
	// float ds = 2*scene_size / (probe_res-1);
	float ds = 2*scene_size / probe_res;
	for (int x_i = 0; x_i < probe_res; x_i++)
		for (int y_i = 0; y_i < probe_res; y_i++)
			for (int z_i = 0; z_i < probe_res; z_i++){
				// if(probe_res == 1) {
				// 	pos = glm::vec3(0,0,0);
				// } else {
				// 	pos = -scene_size + ds* glm::vec3(x_i, y_i ,z_i);
				// }
				glm::vec3 pos = -scene_size + ds * (glm::vec3(0.5) + glm::vec3(x_i, y_i ,z_i));
				world_position.push_back(pos);
			}

	
	// pack all cubemap to a big 2D texture
	glGenTextures(1, &GB_pos_2D);
	glBindTexture(GL_TEXTURE_2D, GB_pos_2D);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, cubemap_res*6*probe_res, cubemap_res*probe_res*probe_res);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenTextures(1, &GB_norm_2D);
	glBindTexture(GL_TEXTURE_2D, GB_norm_2D);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, cubemap_res*6*probe_res, cubemap_res*probe_res*probe_res);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// precomputed SH and solid angle
	// glGenTextures(1, &precomp_SH0123);
	// glBindTexture(GL_TEXTURE_2D, precomp_SH0123);
	// glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, cubemap_res*6, cubemap_res);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glGenTextures(1, &precomp_SH4567);
	// glBindTexture(GL_TEXTURE_2D, precomp_SH4567);
	// glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, cubemap_res*6, cubemap_res);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glGenTextures(1, &precomp_SH8);
	// glBindTexture(GL_TEXTURE_2D, precomp_SH8);
	// glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, cubemap_res*6, cubemap_res);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

SH_volume::~SH_volume()
{
	glDeleteTextures(num_sh_tex, sh_tex);
	// glDeleteTextures(1, &probe_tex);
	glDeleteTextures(probe_radiance.size(), probe_radiance.data());
	glDeleteTextures(probe_GB_norm.size(), probe_GB_norm.data());
	glDeleteTextures(probe_GB_pos.size(), probe_GB_pos.data());
}

void SH_volume::bake()
{
	GLint m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	glViewport(0, 0, cubemap_res, cubemap_res);
	auto& app = App::get();
	gbuffer_shader.bind();
	gbuffer_shader.uniform("model", glm::mat4(1));
	gbuffer_shader.uniform("projection", captureProjection);

	app.captureFB.bind(); //glBindFramebuffer
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemap_res, cubemap_res);


	int num_probe = probe_res * probe_res * probe_res;
	for (int i = 0; i < num_probe; i++) {
		glm::vec3 pos = world_position[i];
		fmt::print("probe_pos: {} {} {}\n", pos[0], pos[1], pos[2]);
		for (int face = 0; face < 6; face++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, probe_GB_pos[i], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, probe_GB_norm[i], 0);
			GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    		glDrawBuffers(2, attachments);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    			std::cout << "Framebuffer not complete!" << std::endl;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			gbuffer_shader.uniform("view", captureViews(pos, face));
			app.scene->render(gbuffer_shader);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);

	// precomputed SH and solid angle
	// precomp_SH_shader.bind();
	// glBindImageTexture(0, precomp_SH0123, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	// glBindImageTexture(1, precomp_SH4567, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	// glBindImageTexture(2, precomp_SH8, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	// glDispatchCompute(8, 8, 6);

	// pack all cubemap to a big 2D texture
	for (int x_i = 0; x_i < probe_res; x_i++)
		for (int y_i = 0; y_i < probe_res; y_i++)
			for (int z_i = 0; z_i < probe_res; z_i++)
				for (int face = 0; face < 6; face++)
				{
					GLint cubemap_index = (x_i * probe_res + y_i) * probe_res + z_i;
					GLint dstX = cubemap_res * (6 * x_i + face);
					GLint dstY = cubemap_res * (probe_res * z_i + y_i);
					glCopyImageSubData(probe_GB_norm[cubemap_index], GL_TEXTURE_CUBE_MAP, 0, 0, 0, face,
					                   GB_norm_2D, GL_TEXTURE_2D, 0, dstX, dstY, 0,
					                   cubemap_res, cubemap_res, 1);
					glCopyImageSubData(probe_GB_pos[cubemap_index], GL_TEXTURE_CUBE_MAP, 0, 0, 0, face,
					                   GB_pos_2D, GL_TEXTURE_2D, 0, dstX, dstY, 0,
					                   cubemap_res, cubemap_res, 1);
				}
}

void SH_volume::print()
{
	int num_probe = probe_res * probe_res * probe_res;

	for (int i = 0; i < num_probe; i++) {
		std::vector<glm::vec4> probe_GB(cubemap_res * cubemap_res, glm::vec4(0.5, 0.5, 0.5, 0.5));
		glBindTexture(GL_TEXTURE_CUBE_MAP, probe_GB_norm[i]);
		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, GL_FLOAT, probe_GB.data());
		fmt::print("probe_GB norm: {} {} {} {}\n", probe_GB[0][0], probe_GB[0][1], probe_GB[0][2], probe_GB[0][3]);
	}

	for (int i = 0; i < num_probe; i++) {
		std::vector<glm::vec4> probe_cpu(cubemap_res * cubemap_res, glm::vec4(0.5, 0.5, 0.5, 0.5));
		glBindTexture(GL_TEXTURE_CUBE_MAP, probe_radiance[i]);
		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, GL_FLOAT, probe_cpu.data());
		fmt::print("probe_radiance: {} {} {} {}\n", probe_cpu[0][0], probe_cpu[0][1], probe_cpu[0][2], probe_cpu[0][3]);
	}

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

void SH_volume::relight()
{
	// auto& app = App::get();
	// int num_probe = probe_res * probe_res * probe_res;
	// relight_shader.bind();
	// relight_shader.uniform("light.intensity", app.cast_light_intensity*glm::vec3(1,1,1));
	// relight_shader.uniform("light.position", glm::vec3{app.cast_light_position[0], app.cast_light_position[1], app.cast_light_position[2]});
	// relight_shader.uniform("light.direction", glm::vec3(1,0,0));
	// relight_shader.uniform("light.cutOff", app.cast_light_cut_off);
	// relight_shader.uniform("ambient.position", glm::vec3(0,0,0));
	// relight_shader.uniform("ambient.intensity", 0.f*glm::vec3(1));
	// relight_shader.uniform("multi_bounce", app.multi_bounce);
	// relight_shader.uniform("atten", app.atten);
	// bind_sh_tex(relight_shader);
	// for (int i = 0; i < num_probe; i++) {
	// 	glBindImageTexture(0, probe_GB_pos[i], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	// 	glBindImageTexture(1, probe_GB_norm[i], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	// 	glBindImageTexture(2, probe_radiance[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	// 	glDispatchCompute(cubemap_res/8, cubemap_res/8, 6);
	// }
}

void SH_volume::project_sh()
{

	relight_project_shader.bind();
	relight_project_shader.uniform("probe_res", probe_res);
	// direct light
	auto& app = App::get();
	relight_project_shader.uniform("light.intensity", app.cast_light_intensity*glm::vec3(1,1,1));
	relight_project_shader.uniform("light.position", glm::vec3{app.cast_light_position[0], app.cast_light_position[1], app.cast_light_position[2]});
	relight_project_shader.uniform("light.direction", glm::vec3(1,0,0));
	relight_project_shader.uniform("light.cutOff", app.cast_light_cut_off);
	relight_project_shader.uniform("ambient.position", glm::vec3(0,0,0));
	relight_project_shader.uniform("ambient.intensity", 0.f*glm::vec3(1));
	relight_project_shader.uniform("multi_bounce", app.multi_bounce);
	relight_project_shader.uniform("atten", app.atten);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app.sky_shadow.depthMap);
	relight_project_shader.uniform("shadowMap", 0);
	relight_project_shader.uniform("lightSpaceMatrix", app.sky_shadow.lightSpaceMatrix);
	relight_project_shader.uniform("sky.intensity", glm::vec3(app.sky_intensity));
	relight_project_shader.uniform("sky.direction", app.sky_shadow.direction);
	//multi bounce
	bind_sh_tex(relight_project_shader);
	// SH project
	for (int i = 0; i < num_sh_tex; i++) {
		glBindImageTexture(i, sh_tex[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	}
	// glActiveTexture(GL_TEXTURE0 + 71);
	// glBindTexture(GL_TEXTURE_2D, precomp_SH0123);
	// relight_project_shader.uniform("precomp_SH0123", 71);
	// glActiveTexture(GL_TEXTURE0 + 72);
	// glBindTexture(GL_TEXTURE_2D, precomp_SH4567);
	// relight_project_shader.uniform("precomp_SH4567", 72);
	// glActiveTexture(GL_TEXTURE0 + 73);
	// glBindTexture(GL_TEXTURE_2D, precomp_SH8);
	// relight_project_shader.uniform("precomp_SH8", 73);
	glActiveTexture(GL_TEXTURE0 + 74);
	glBindTexture(GL_TEXTURE_2D, GB_pos_2D);
	relight_project_shader.uniform("WorldPos", 74);
	glActiveTexture(GL_TEXTURE0 + 75);
	glBindTexture(GL_TEXTURE_2D, GB_norm_2D);
	relight_project_shader.uniform("Normal", 75);

	glDispatchCompute(8, 64, 1);

	// project_shader.bind();

	// // glActiveTexture(GL_TEXTURE0 + 71);
	// // glBindTexture(GL_TEXTURE_CUBE_MAP, precomp_SH0123);
	// // project_shader.uniform("precomp_SH0123", 71);
	// // glActiveTexture(GL_TEXTURE0 + 72);
	// // glBindTexture(GL_TEXTURE_CUBE_MAP, precomp_SH4567);
	// // project_shader.uniform("precomp_SH4567", 72);
	// // glActiveTexture(GL_TEXTURE0 + 73);
	// // glBindTexture(GL_TEXTURE_CUBE_MAP, precomp_SH8);
	// // project_shader.uniform("precomp_SH8", 73);

	// for (int i = 0; i < num_sh_tex; i++) {
	// 	glBindImageTexture(i, sh_tex[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	// }

	// int num_probe = probe_res * probe_res * probe_res;
	// int texture_unit = 0;
	// for (int x_i = 0; x_i < probe_res; x_i++)
	// 	for (int y_i = 0; y_i < probe_res; y_i++)
	// 		for (int z_i = 0; z_i < probe_res; z_i++)
	// 		{
	// 			project_shader.uniform("probe_xid", x_i);
	// 			project_shader.uniform("probe_yid", y_i);
	// 			project_shader.uniform("probe_zid", z_i);

	// 			GLint cubemap_index = (x_i * probe_res + y_i) * probe_res + z_i;
	// 			glActiveTexture(GL_TEXTURE0+texture_unit);
	// 			glBindTexture(GL_TEXTURE_CUBE_MAP, probe_radiance[cubemap_index]);
	// 			project_shader.uniform("environment", texture_unit);
	// 			// glBindImageTexture(num_sh_tex, probe_radiance[cubemap_index], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);


	// 			glDispatchCompute(1, 1, 1);
	// 			// texture_unit++; // it is necessary?
	// 			// if (texture_unit == 70) {
	// 			// 	texture_unit = 0;
	// 			// 	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	// 			// }
	// 		}
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

