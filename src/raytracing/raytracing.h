#pragma once
#include <embree3/rtcore.h>
#include "opengl/gl.h"

extern RTCDevice device;

class RTScene{
public:
    RTScene(Mesh &gl_mesh);
    ~RTScene();
    RTCScene scene;
private:
};

void raytrace(const RTScene& rtscene);
