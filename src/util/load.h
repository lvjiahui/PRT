#pragma once
#include <vector>
#include <string>
#include "opengl/gl.h"

GLuint loadCubemap(std::vector<std::string> faces);
GLuint load_hdr(std::string path);

