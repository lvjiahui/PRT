#include "volume.h"
#include "platform/app.h"

SH_volume::SH_volume(int _res) : res(_res)
{
	glGenTextures(num_tex, sh_tex);
	for (int i = 0; i < num_tex; i++) {
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, res, res, res, 0, GL_RGBA, GL_FLOAT, nullptr);
	}

}

void SH_volume::print_sh()
{
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	std::vector<glm::vec4> sh_cpu(res * res * res, glm::vec4(0, 0, 0, 0));
	for (int i = 0; i < num_tex; i++) {
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
		glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, sh_cpu.data());
		fmt::print("{} {} {} {}\n", sh_cpu[0][0], sh_cpu[0][1], sh_cpu[0][2], sh_cpu[0][3]);
	}
}

void SH_volume::project_sh(ComputeShader& shader)
{
    shader.bind();
	shader.uniform("environment", App::get().EnvMap("environment").active());
	for (int i = 0; i < num_tex; i++) {
	    glBindImageTexture(i, sh_tex[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    }
	glDispatchCompute(1, 1, 1);

}

void SH_volume::bind_sh_tex(Shader& shader)
{
	shader.bind();
	for (int i = 0; i < num_tex; i++) {
		int tex_unit = 16 - num_tex + i;
		glActiveTexture(GL_TEXTURE0 + tex_unit);
		glBindTexture(GL_TEXTURE_3D, sh_tex[i]);
		shader.uniform("SH_volume" + std::to_string(i), tex_unit);
	}
}
