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


#define NONCOPYABLE(Type) Type(const Type&)=delete; Type& operator=(const Type&)=delete

namespace fs = std::filesystem;

class RenderShader;
namespace Shaders {
    extern RenderShader brdfShader;
    extern RenderShader screenShader;
    extern RenderShader envShader;
    extern RenderShader castlightShader;
} // namespace Shaders

class Shader {
public:
    NONCOPYABLE(Shader);
    Shader() {};
    Shader(Shader &&src);
    ~Shader();
    void load(std::vector<fs::path> shader_paths);

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
    virtual void create_shader() = 0;

protected:
    GLuint loc(std::string name) const;
    static bool check_compiled(GLuint shader, std::string code={});
    static bool check_linked(GLuint program);

    GLuint program = 0;
    std::vector<std::string> shader_codes;
    std::vector<GLuint> shaders;

    void destroy();
};

class RenderShader : public Shader {
public:
    RenderShader() {};
    RenderShader(std::initializer_list<fs::path> shader_paths) { load(shader_paths); }
    void create_shader() override;
};

class ComputeShader : public Shader {
public:
    ComputeShader() {};
    ComputeShader(std::initializer_list<fs::path> shader_paths) { load(shader_paths); }
    void create_shader() override;
};

class Mesh {
public:
    NONCOPYABLE(Mesh);
    typedef GLuint Index;
    struct Vert {
        glm::vec3 pos;
        glm::vec3 norm;
        GLfloat sh_coeff[9];
    };
    glm::mat4 Mat_model = glm::mat4(1);

    Mesh();
    Mesh(std::vector<Vert> &&vertices, std::vector<Index> &&indices);
    Mesh(Mesh &&src);
    ~Mesh();

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
    NONCOPYABLE(Framebuffer);
    Framebuffer();
    void bind();
    ~Framebuffer();
private:
    GLuint framebufferobject;
    GLuint depthbuffer;
};

class Tex2D {
public:
    NONCOPYABLE(Tex2D);
    Tex2D() = default;
    Tex2D(Tex2D &&src);
    ~Tex2D();


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
    NONCOPYABLE(Quad);
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
    NONCOPYABLE(SkyBox);
    RenderShader skyboxShader = RenderShader{ fs::path{"src/shaders/skybox.vert"}, fs::path{"src/shaders/skybox.frag"} };
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
    RenderShader rectangleShader = RenderShader{ fs::path{"src/shaders/skybox.vert"}, fs::path{"src/shaders/rectangle2cube.frag"} };
    RenderShader irradianceShader = RenderShader{ fs::path{"src/shaders/skybox.vert"}, fs::path{"src/shaders/irradiance.frag"} };
    RenderShader prefilterShader = RenderShader{ fs::path{"src/shaders/skybox.vert"}, fs::path{"src/shaders/prefilter.frag"} };

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

class Model;

class Paral_Shadow{
public:
    NONCOPYABLE(Paral_Shadow);
    Paral_Shadow();
    ~Paral_Shadow();

    void render(Model&);
    void set_dir(float, float);
    RenderShader shadow_depth_shader{ fs::path{"src/shaders/shadow_depth.vert"}, fs::path{"src/shaders/shadow_depth.frag"} };
    glm::vec3 direction;
    glm::mat4 lightSpaceMatrix{1};
    // const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    const GLuint SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    GLuint depthMap;
    GLuint depthMapFBO;
    float near_plane = 0.1, far_plane = 100;
};