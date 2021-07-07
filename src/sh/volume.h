#pragma once

#include "opengl/gl.h"

class SH_volume {
public:
    SH_volume(int res);
    void print_sh();
    void project_sh(ComputeShader&);
    void bind_sh_tex(Shader&);
    int res;
    static const int num_tex = 7;
    GLuint sh_tex[num_tex];
};