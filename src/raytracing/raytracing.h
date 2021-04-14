#pragma once
#include <embree3/rtcore.h>
#include "opengl/gl.h"


class RTScene{
public:
    RTScene(Mesh &gl_mesh);
    ~RTScene();
    RTCScene scene;
private:
};

void raytrace(const RTScene& rtscene);
void bake_AO(Mesh& gl_mesh);
void bake_SH(Mesh& gl_mesh);
