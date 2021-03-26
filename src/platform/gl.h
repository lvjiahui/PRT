#pragma once

#include <glad/glad.h>
#include <string>
#include <fmt/core.h>

#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi


namespace Shaders {
extern const std::string mesh_v, mesh_f;

} // namespace Shaders

class Shader {
public:
    Shader();
    Shader(std::string vertex_file, std::string fragment_file);
    Shader(const Shader &src) = delete;
    Shader(Shader &&src);
    ~Shader();

    void operator=(const Shader &src) = delete;
    void operator=(Shader &&src);

    void bind() const;
    void load(std::string vertex, std::string fragment);

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
    GLuint loc(std::string name) const;
    static bool validate(GLuint program);

    GLuint program = 0;
    GLuint v = 0, f = 0;

    void destroy();
};


class Mesh {
public:
    typedef GLuint Index;
    struct Vert {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    Mesh();
    Mesh(std::vector<Vert> &&vertices, std::vector<Index> &&indices);
    Mesh(const Mesh &src) = delete;
    Mesh(Mesh &&src);
    ~Mesh();

    void operator=(const Mesh &src) = delete;
    void operator=(Mesh &&src);

    /// Assumes proper shader is already bound
    void render(Shader &shader);
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