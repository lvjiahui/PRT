#pragma once
#include <vector>
#include <string>
#include "opengl/gl.h"

std::vector<float> rotate_cos_lobe(const glm::vec3& normal);
std::vector<glm::vec3> rotate_sh(const std::vector<glm::vec3>&, const glm::mat4&);
Tex2D load_hdr(std::string path);

