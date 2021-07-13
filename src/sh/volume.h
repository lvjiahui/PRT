#pragma once

#include "opengl/gl.h"



class SH_volume {
public:
    SH_volume(GLsizei probe_res);
    SH_volume(const SH_volume&) = delete;
    void operator=(const SH_volume&) = delete;
    ~SH_volume();
    void bake();
    void relight();
    void print();
    void project_sh();
    void bind_sh_tex(Shader&);
    void bind_sh_tex(ComputeShader& shader);
    // ComputeShader project_shader{ fs::path{"src/shaders/image_projectSH.comp"} };
    // ComputeShader project_shader{ fs::path{"src/shaders/precomp_projectSH.comp"} };
    // ComputeShader project_shader{ fs::path{"src/shaders/projectSH.comp"} };
    // ComputeShader relight_shader{ fs::path{"src/shaders/relight.comp"} };
    ComputeShader relight_project_shader{ fs::path{"src/shaders/relight_projectSH.comp"} };
    // ComputeShader relight_project_shader{ fs::path{"src/shaders/precomp_relight_projectSH.comp"} };
    // ComputeShader precomp_SH_shader{ fs::path{"src/shaders/precomp_SH.comp"} };
    RenderShader gbuffer_shader{ fs::path{"src/shaders/mesh.vert"}, fs::path{"src/shaders/g_buffer.frag"} };
    SkyBox skybox;

    GLsizei probe_res;
    static const GLsizei cubemap_res = 64;
    float scene_size = 6;
    static const int num_sh_tex = 7;
    GLuint sh_tex[num_sh_tex];
    // GLuint probe_tex;
    // GLuint precomp_SH0123;
    // GLuint precomp_SH4567;
    // GLuint precomp_SH8;
    GLuint GB_pos_2D;
    GLuint GB_norm_2D;
    std::vector<GLuint> probe_radiance;
    std::vector<GLuint> probe_GB_pos;
    std::vector<GLuint> probe_GB_norm;
    std::vector<glm::vec3> world_position;
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f);
    
    glm::mat4 captureViews(glm::vec3 position, int face);
};