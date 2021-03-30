#pragma once
#include <vector>
#include <string>
#include "opengl/gl.h"

GLuint loadCubemap(std::vector<std::string> faces);
GLuint load_hdr(std::string path);
GLuint equirectangular_to_cubemap(SkyBox &skybox, size_t w = 512, size_t h = 512);
GLuint irradiance_map(SkyBox &skybox, size_t w = 32, size_t h = 32);
