#pragma once

#include "opengl/gl.h"



class SH_volume {
public:
    SH_volume() {};
    SH_volume(const SH_volume&) = delete;
    void operator=(const SH_volume&) = delete;
    ~SH_volume();
    void init (GLsizei, glm::vec3);
    void bake();
    void precompute();
    void relight();
    void print();
    void project_sh();
    void bind_sh_tex(Shader&);
    void set_volume_filter(int FILTER);
    // ComputeShader project_shader{ fs::path{"src/shaders/image_projectSH.comp"} };
    ComputeShader project_shader{ fs::path{"src/shaders/precomp_projectSH.comp"} };
    // ComputeShader project_shader{ fs::path{"src/shaders/projectSH.comp"} };
    ComputeShader relight_shader{ fs::path{"src/shaders/relight.comp"} };
    // ComputeShader relight_project_shader{ fs::path{"src/shaders/relight_projectSH.comp"} };
    // ComputeShader relight_project_shader{ fs::path{"src/shaders/precomp_relight_projectSH.comp"} };
    // ComputeShader precomp_SH_shader{ fs::path{"src/shaders/precomp_SH.comp"} };
    RenderShader gbuffer_shader{ fs::path{"src/shaders/mesh.vert"}, fs::path{"src/shaders/g_buffer.frag"} };
    SkyBox skybox;

    glm::vec3 scene_size;
    GLsizei probe_res;
    // per probe data
    static const int num_sh_tex = 7;
    GLuint sh_tex[num_sh_tex]; //3D texture
    GLuint probe_range; //3D texture
    std::vector<glm::vec3> world_position;
    // buffer
    GLuint transfer_buffer, transfer_tex;
    GLuint rad_buffer, rad_tex;
    GLuint ID_buffer, ID_tex;
    GLuint primitive_buffer, primitive_tex;
    int num_primitive;
    // cubemap
    static const GLsizei cubemap_res = 64;
    GLuint GBuffer_pos;
    GLuint GBuffer_norm;
    GLuint GBuffer_ID;
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f);
    
    glm::mat4 captureViews(glm::vec3 position, int face);
};