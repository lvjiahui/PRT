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

class Shader;
class ComputeShader;
namespace Shaders {
    extern Shader brdfShader;
    extern Shader screenShader;
    extern Shader envShader;
    extern Shader castlightShader;
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
    void uniform(std::string name, int count, const glm::vec3 items[]) const;
    void uniform_block(std::string name, GLuint i) const;

private:
    void load(std::string vertex_code, std::string fragment_code);
    GLuint loc(std::string name) const;
    static bool validate(GLuint program, std::string code={});

    GLuint program = 0;
    GLuint v = 0, f = 0;

    void destroy();
};

class ComputeShader {
public:
    ComputeShader() {};
    explicit ComputeShader(fs::path shader_path);
    ComputeShader(const ComputeShader &src) = delete;
    void operator=(ComputeShader&& src);
    void operator=(const ComputeShader &src) = delete;
    void bind() const;
    ~ComputeShader();
    void uniform(std::string name, const glm::mat4 &mat) const;
    void uniform(std::string name, glm::vec3 vec3) const;
    void uniform(std::string name, glm::vec2 vec2) const;
    void uniform(std::string name, GLint i) const;
    void uniform(std::string name, GLuint i) const;
    void uniform(std::string name, GLfloat f) const;
    void uniform(std::string name, bool b) const;
    void uniform(std::string name, int count, const glm::vec2 items[]) const;
    void uniform(std::string name, int count, const glm::vec3 items[]) const;
    GLuint program_id() { return program; };
private:
    GLuint loc(std::string name) const;
    static bool validate(GLuint program, std::string code={});
    void destroy();
    GLuint shader = 0;
    GLuint program = 0;
};

class Mesh {
public:
    typedef GLuint Index;
    struct Vert {
        glm::vec3 pos;
        glm::vec3 norm;
        GLfloat sh_coeff[9];
    };
    glm::mat4 Mat_model = glm::mat4(1);

    Mesh();
    Mesh(std::vector<Vert> &&vertices, std::vector<Index> &&indices);
    Mesh(const Mesh &src) = delete;
    Mesh(Mesh &&src);
    ~Mesh();

    void operator=(const Mesh &src) = delete;
    void operator=(Mesh &&src);

    void render(Shader& shader);
    void instance_render(int num);
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

class Framebuffer{
public:
    Framebuffer();
    Framebuffer(const Framebuffer &src) = delete;
    void operator=(const Framebuffer &src) = delete;
    void bind();
    ~Framebuffer();
private:
    GLuint framebufferobject;
    GLuint depthbuffer;
};

class Tex2D {
public:
    Tex2D() = default;
    Tex2D(const Tex2D &src) = delete;
    Tex2D(Tex2D &&src);
    ~Tex2D();

    void operator=(const Tex2D &src) = delete;
    void operator=(Tex2D &&src);

    void imagei(int w, int h, unsigned char *img = nullptr); //source:rgba
    void imagef(int w, int h, float *img = nullptr); //source:rgb
    GLuint get_id() const;
    GLint active(int unit = 0) const;
    template<typename Renderable>
    void render_to(Renderable &renderable){
        GLint m_viewport[4];
        glGetIntegerv(GL_VIEWPORT, m_viewport);

        glViewport(0, 0, w, h);
        App::get().captureFB.bind(); //glBindFramebuffer
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderable.render();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
    }
    int w = 0, h = 0;
private:
    void setup() const;
    GLuint id = 0;
};

class CubeMap {
public:
    CubeMap() = default;
    CubeMap(std::vector<std::string> paths, bool mipmap = false);
    CubeMap(size_t w, size_t h, bool mipmap = false);
    CubeMap(CubeMap&&);
    CubeMap& operator=(CubeMap&&);
    GLint active(int unit = -1);
    template<typename Renderable>
    void render_to(Renderable& renderable, GLuint view, GLuint mip = 0) {
        GLint m_viewport[4];
        glGetIntegerv(GL_VIEWPORT, m_viewport);

        unsigned int mipWidth = w * std::pow(0.5, mip);
        unsigned int mipHeight = h * std::pow(0.5, mip);
        glViewport(0, 0, mipWidth, mipHeight);
        App::get().captureFB.bind(); //glBindFramebuffer
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + view, textName, mip);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderable.render();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
    }
    void generateMipmap();
    ~CubeMap();
    int w = 0, h = 0;
    bool mipmap = false;
    GLuint textName = 0; // 0 is a reserved texture name
private:
    int default_unit = 1;
    void setup();
    void destroy();
};


class Quad{
public:
    Quad();
    void render();
private:
    GLuint quadVAO = 0, quadVBO;
    float quadVertices[20] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
};

class SkyBox {
public:
    Shader skyboxShader = Shader{ fs::path{"src/opengl/skybox.vert"}, fs::path{"src/opengl/skybox.frag"} };
    SkyBox();
    void create();
    void setShader();
    void render();
    ~SkyBox();
    glm::mat4 Mat_rotate = glm::mat4(1);
private:
    GLuint skyboxVAO = 0, skyboxVBO;

    const float skyboxVertices[108] = {
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
    };
};


class LightProbe {
public:
    LightProbe(SkyBox &skybox);
    void prefilter(CubeMap& cubemap);
    void irradiance(CubeMap& cubemap);
    void equirectangular_to_cubemap(Tex2D &rectTex, CubeMap& cubemap);

private:
    SkyBox &skybox;
    Shader rectangleShader = Shader{ fs::path{"src/opengl/skybox.vert"}, fs::path{"src/opengl/rectangle2cube.frag"} };
    Shader irradianceShader = Shader{ fs::path{"src/opengl/skybox.vert"}, fs::path{"src/opengl/irradiance.frag"} };
    Shader prefilterShader = Shader{ fs::path{"src/opengl/skybox.vert"}, fs::path{"src/opengl/prefilter.frag"} };

    // set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[6] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };
};
