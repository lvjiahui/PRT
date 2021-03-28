#pragma once

#include <glad/glad.h>
#include <string>
#include <filesystem>
#include <fmt/core.h>

#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

namespace fs = std::filesystem;

namespace Shaders {
extern const std::string SkyBox_v, SkyBox_f;

} // namespace Shaders

class Shader {
public:
    Shader();
    Shader(std::string vertex_code, std::string fragment_code);
    Shader(fs::path vertex_path, fs::path fragment_path);
    Shader(const Shader &src) = delete;
    Shader(Shader &&src);
    ~Shader();

    void operator=(const Shader &src) = delete;
    void operator=(Shader &&src);

    void bind() const;

    void uniform(std::string name, const glm::mat4 &mat) const;
    void uniform(std::string name, glm::vec3 vec3) const;
    void uniform(std::string name, glm::vec2 vec2) const;
    void uniform(std::string name, GLint i) const;
    void uniform(std::string name, GLuint i) const;
    void uniform(std::string name, GLfloat f) const;
    void uniform(std::string name, bool b) const;
    void uniform(std::string name, int count, const glm::vec2 items[]) const;
    void uniform_block(std::string name, GLuint i) const;

private:
    void load(std::string vertex_code, std::string fragment_code);
    GLuint loc(std::string name) const;
    static bool validate(GLuint program);

    GLuint program = 0;
    GLuint v = 0, f = 0;

    void destroy();
};


class Mesh {
public:
    Shader meshShader = Shader{ fs::path{"src/opengl/mesh.vert"}, fs::path{"src/opengl/mesh.frag"} };
    typedef GLuint Index;
    struct Vert {
        glm::vec3 pos;
        glm::vec3 norm;
    };
    glm::mat4 Mat_model = glm::mat4(1);

    Mesh();
    Mesh(std::vector<Vert> &&vertices, std::vector<Index> &&indices);
    Mesh(const Mesh &src) = delete;
    Mesh(Mesh &&src);
    ~Mesh();

    void operator=(const Mesh &src) = delete;
    void operator=(Mesh &&src);

    void render();
    void recreate(std::vector<Vert> &&vertices, std::vector<Index> &&indices);
    std::vector<Vert> &edit_verts();
    std::vector<Index> &edit_indices();

    const std::vector<Vert> &verts() const;
    const std::vector<Index> &indices() const;
    GLuint tris() const;

private:
    void update();
    void create();
    void destroy();

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint n_elem = 0;
    bool dirty = true;

    std::vector<Vert> _verts;
    std::vector<Index> _idxs;

};

class SkyBox {
public:
    Shader skyboxShader = Shader{ Shaders::SkyBox_v, Shaders::SkyBox_f };
    SkyBox();
    void create();
    void render();
    ~SkyBox();
private:
    GLuint skyboxVAO = 0, skyboxVBO;

    float skyboxVertices[108] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };;
};