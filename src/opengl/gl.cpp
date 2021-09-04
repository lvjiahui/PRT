#include "opengl/gl.h"
#include "platform/app.h"
#include "util/log.h"
#include "util/util.h"
#include <fstream>
#include <sstream>
#include <sf_libs/stb_image.h>
#include <sf_libs/Shadinclude.hpp>




// Shader::Shader(std::string vertex_code, std::string fragment_code) { create(vertex_code, fragment_code); }

void Shader::load(std::vector<fs::path> shader_paths)
{

    for(auto &path : shader_paths){
        std::string shader_code = Shadinclude::load(path.string(), "#include");
        shader_codes.push_back(shader_code);
        //save to shaders/generated/
        std::ofstream out_file("src/shaders/generated/" + path.filename().string());
		if (!out_file.is_open())
		{
			fmt::print(stderr, "cannot write file to src/shaders/generated/ \n");
		} else {
            out_file << shader_code;
            out_file.close();
        }
    }

    program = glCreateProgram();
    create_shader();
    assert(shaders.size() == shader_codes.size());

    for(int i = 0; i < shaders.size(); i++){
        const GLchar *code_c = shader_codes[i].c_str();
        glShaderSource(shaders[i], 1, &code_c, NULL);
        glCompileShader(shaders[i]);
        if (!check_compiled(shaders[i], code_c)) {
            destroy();
            return;
        }
        glAttachShader(program, shaders[i]);
    }
    
    glLinkProgram(program);
    if (!check_linked(program)) {
        destroy();
        return;
    }
}

Shader::Shader(Shader &&src) {
    program = src.program;
    src.program = 0;
    shader_codes = std::move(src.shader_codes);
    shaders = std::move(src.shaders);
}

void Shader::operator=(Shader &&src) {
    destroy();
    program = src.program;
    src.program = 0;
    shader_codes = std::move(src.shader_codes);
    shaders = std::move(src.shaders);
}

Shader::~Shader() { destroy(); }

void Shader::bind() const { glUseProgram(program); }

void Shader::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if (!glUseProgram)
        return;

    glUseProgram(0);
    for(auto shader : shaders)
        glDeleteShader(shader);
    glDeleteProgram(program);
    shader_codes.clear();
    shaders.clear();
}

void Shader::uniform_block(std::string name, GLuint i) const {
    GLuint idx = glGetUniformBlockIndex(program, name.c_str());
    glUniformBlockBinding(program, idx, i);
}

void Shader::uniform(std::string name, int count, const glm::vec2 items[]) const {
    glUniform2fv(loc(name), count, (GLfloat *)items);
}

void Shader::uniform(std::string name, int count, const glm::vec3 items[]) const {
    glUniform3fv(loc(name), count, (GLfloat *)items);
}

void Shader::uniform(std::string name, GLfloat fl) const { glUniform1f(loc(name), fl); }

void Shader::uniform(std::string name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(loc(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::uniform(std::string name, glm::vec3 vec3) const { glUniform3fv(loc(name), 1, &vec3[0]); }

void Shader::uniform(std::string name, glm::ivec3 ivec3) const { glUniform3iv(loc(name), 1, &ivec3[0]); }

void Shader::uniform(std::string name, glm::vec2 vec2) const { glUniform2fv(loc(name), 1, &vec2[0]); }

void Shader::uniform(std::string name, GLint i) const { glUniform1i(loc(name), i); }

void Shader::uniform(std::string name, GLuint i) const { glUniform1ui(loc(name), i); }

void Shader::uniform(std::string name, bool b) const { glUniform1i(loc(name), b); }

GLuint Shader::loc(std::string name) const { 
    auto location = glGetUniformLocation(program, name.c_str());
    if(location == -1){
        fmt::print("can not locale {}\n", name);
        assert(false);
    }
    return location; 
}


bool Shader::check_compiled(GLuint shader, std::string code) {

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {

        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        GLchar *msg = new GLchar[len];
        glGetShaderInfoLog(shader, len, &len, msg);

        warn("Shader %d failed to compile: %s \n %s", shader, msg, code.c_str());
        delete[] msg;

        return false;
    }
    return true;
}

bool Shader::check_linked(GLuint program) {

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {

        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

        GLchar *msg = new GLchar[len];
        glGetProgramInfoLog(program, len, &len, msg);

        warn("program %d failed to link: %s \n", program, msg);
        delete[] msg;

        return false;
    }
    return true;
}

void RenderShader::create_shader(){
    shaders.push_back(glCreateShader(GL_VERTEX_SHADER));
    shaders.push_back(glCreateShader(GL_FRAGMENT_SHADER));
}

void ComputeShader::create_shader(){
    shaders.push_back(glCreateShader(GL_COMPUTE_SHADER));
}

Mesh::Mesh() { create(); }

Mesh::Mesh(std::vector<Vert>&& vertices, std::vector<Index>&& indices) {
    create();
    recreate(std::move(vertices), std::move(indices));
}

Mesh::Mesh(Mesh&& src) {
    vao = src.vao;
    src.vao = 0;
    ebo = src.ebo;
    src.ebo = 0;
    vbo = src.vbo;
    src.vbo = 0;
    dirty = src.dirty;
    src.dirty = true;
    n_elem = src.n_elem;
    src.n_elem = 0;
    _verts = std::move(src._verts);
    _idxs = std::move(src._idxs);
}

void Mesh::operator=(Mesh&& src) {
    destroy();
    vao = src.vao;
    src.vao = 0;
    vbo = src.vbo;
    src.vbo = 0;
    ebo = src.ebo;
    src.ebo = 0;
    dirty = src.dirty;
    src.dirty = true;
    n_elem = src.n_elem;
    src.n_elem = 0;
    _verts = std::move(src._verts);
    _idxs = std::move(src._idxs);
}

Mesh::~Mesh() { destroy(); }

void Mesh::create() {
    // Hack to let stuff get created for headless mode
    if (!glGenVertexArrays)
        return;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)sizeof(glm::vec3));
    glEnableVertexAttribArray(1);

    for (GLuint i = 0; i < 9; i++){
        glVertexAttribPointer(i+2, 1, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)(2*sizeof(glm::vec3) + (i*sizeof(GLfloat))));
        glEnableVertexAttribArray(i+2);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBindVertexArray(0);
}

void Mesh::destroy() {
    // Hack to let stuff get destroyed for headless mode
    if (!glDeleteBuffers)
        return;

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    ebo = vao = vbo = 0;
}

void Mesh::update() {
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * _verts.size(), _verts.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Index) * _idxs.size(), _idxs.data(),
        GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    dirty = false;
}

void Mesh::recreate(std::vector<Vert>&& vertices, std::vector<Index>&& indices) {

    dirty = true;
    _verts = std::move(vertices);
    _idxs = std::move(indices);
    n_elem = (GLuint)_idxs.size();
}

GLuint Mesh::tris() const { return n_elem / 3; }

std::vector<Mesh::Vert>& Mesh::edit_verts() {
    dirty = true;
    return _verts;
}

std::vector<Mesh::Index>& Mesh::edit_indices() {
    dirty = true;
    return _idxs;
}

const std::vector<Mesh::Vert>& Mesh::verts() const { return _verts; }

const std::vector<Mesh::Index>& Mesh::indices() const { return _idxs; }

void Mesh::render(Shader& shader) {
    if (dirty)
        update();
	shader.uniform("model", Mat_model);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, n_elem, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Framebuffer::Framebuffer(){
    // setup framebuffer
    // ----------------------
    glGenFramebuffers(1, &framebufferobject);
    glGenRenderbuffers(1, &depthbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferobject);
    glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferobject);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Framebuffer::bind(){
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferobject);
}

Framebuffer::~Framebuffer(){
    glDeleteFramebuffers(1, &framebufferobject);
    glDeleteRenderbuffers(1, &depthbuffer);
}


Tex2D::Tex2D(Tex2D &&src) {
    w = src.w;
    h = src.h;
    id = src.id;
    src.id = 0;
}

Tex2D::~Tex2D() {
    if (id)
        glDeleteTextures(1, &id);
    id = 0;
}

void Tex2D::operator=(Tex2D &&src) {
    if (id)
        glDeleteTextures(1, &id);
    w = src.w;
    h = src.h;
    id = src.id;
    src.id = 0;
}

GLint Tex2D::active(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
    return unit;
}

void Tex2D::imagei(int _w, int _h, unsigned char *img) {
    w = _w, h = _h;
    if (!id)
        glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    setup();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Tex2D::imagef(int _w, int _h, float *img) {
    w = _w, h = _h;
    if (!id)
        glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGB, GL_FLOAT, img);
    setup();
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Tex2D::get_id() const
{
    return id;
}

void Tex2D::setup() const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


CubeMap::CubeMap(size_t w, size_t h, bool mipmap) : w(w), h(h), mipmap(mipmap)
{
    glGenTextures(1, &textName);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textName);
    for (GLuint i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    setup();
}

CubeMap::CubeMap(std::vector<std::string> paths, bool mipmap) : mipmap(mipmap)
{
    glGenTextures(1, &textName);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textName);
    int nrChannels;
    for (GLuint i = 0; i < paths.size(); i++)
    {
        unsigned char *data = stbi_load(paths[i].c_str(), &w, &h, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            fmt::print("Cubemap texture failed to load at path: {}\n", paths[i] );
        }
    }
    setup();
}

void CubeMap::setup(){
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if (mipmap)
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (mipmap)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
};

CubeMap::CubeMap(CubeMap&& other)
{
    *this = std::move(other);
}

CubeMap& CubeMap::operator=(CubeMap&& other)
{
    destroy();
    w = other.w;
    h = other.h;
    mipmap = other.mipmap;
    textName = other.textName;
    other.textName = 0;
    return *this;
}

GLint CubeMap::active(int unit)
{
    if (unit == -1)
        unit = default_unit;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textName);
    return unit;
}

void CubeMap::generateMipmap()
{
    mipmap = true;
    glBindTexture(GL_TEXTURE_CUBE_MAP, textName);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}


CubeMap::~CubeMap()
{
    destroy();
}
void CubeMap::destroy()
{
    glDeleteTextures(1, &textName);
}

Quad::Quad()
{
    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void Quad::render()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

SkyBox::SkyBox()
{
    create();
}

void SkyBox::create()
{
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void SkyBox::setShader()
{
    auto& app = App::get();
    skyboxShader.bind();
    skyboxShader.uniform("environment", app.EnvMap().active());
    skyboxShader.uniform("view", app.camera.GetViewMatrix());
    skyboxShader.uniform("projection", app.Mat_projection);
    skyboxShader.uniform("tonemap", app.tonemap);
    skyboxShader.uniform("gamma", app.gamma);
    skyboxShader.uniform("lod", app.lod);
    skyboxShader.uniform("white_bk", app.white_bk);
    skyboxShader.uniform("envRotate", glm::transpose(Mat_rotate));
}


void SkyBox::render()
{
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS); // set depth function back to default
}

SkyBox::~SkyBox()
{
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    skyboxVAO = skyboxVBO = 0;
}

LightProbe::LightProbe(SkyBox& skybox) : skybox(skybox)
{}

void LightProbe::prefilter(CubeMap& cubemap)
{
    if (!cubemap.mipmap) {
        fmt::print("cubemap should be mipmap\n");
        return;
    }

    prefilterShader.bind();
    prefilterShader.uniform("environment", App::get().EnvMap("environment").active());
    prefilterShader.uniform("projection", captureProjection);
    for (GLuint view = 0; view < 6; ++view)
    {
        prefilterShader.uniform("view", captureViews[view]);
        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader.uniform("roughness", roughness);
            cubemap.render_to(skybox, view, mip);
        }
    };
}

void LightProbe::irradiance(CubeMap& cubemap)
{
    irradianceShader.bind();
    irradianceShader.uniform("environment", App::get().EnvMap("environment").active());
    irradianceShader.uniform("projection", captureProjection);
    for (GLuint view = 0; view < 6; ++view)
    {
        irradianceShader.uniform("view", captureViews[view]);
        cubemap.render_to(skybox, view);
    }
}

void LightProbe::equirectangular_to_cubemap(Tex2D &rectTex, CubeMap& cubemap)
{
    rectangleShader.bind();
    rectangleShader.uniform("equirectangularMap", rectTex.active());
    rectangleShader.uniform("projection", captureProjection);
    for (GLuint view = 0; view < 6; ++view)
    {
        rectangleShader.uniform("view", captureViews[view]);
        cubemap.render_to(skybox, view);
    }
}


Paral_Shadow::Paral_Shadow(){
    near_plane = 0.1, far_plane = 60;
    // configure depth map FBO
    // -----------------------
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Paral_Shadow::~Paral_Shadow(){
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
}

void Paral_Shadow::set_dir(float up, float dir){
    float theta = PI * up;
	float phi = 2*PI * dir;
    direction = glm::vec3(sin(theta) * sin(phi), cos(theta), sin(theta) * cos(phi));
    auto lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
    glm::vec3 up_vec = glm::vec3(0.0, 1.0, 0.0);
    if (up < 0.1 || up > 0.9) up_vec = glm::vec3(0.0, 0.0, 1.0);
    auto lightView = glm::lookAt(30.f*direction, glm::vec3(0.0f), up_vec);
    lightSpaceMatrix = lightProjection * lightView;
}

void Paral_Shadow::render(Model& scene){
    shadow_depth_shader.bind();
    shadow_depth_shader.uniform("lightSpaceMatrix", lightSpaceMatrix);

    GLint m_viewport[4];
    glGetIntegerv(GL_VIEWPORT, m_viewport);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    scene.render(shadow_depth_shader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
}
