#pragma once
#include <embree3/rtcore.h>
#include "scene/model.h"


class RTScene{
public:
    RTScene(Model &gl_model);
    ~RTScene();
    RTCScene scene;
private:
};

struct Volume_weight{
    std::vector<glm::vec4> weight0123;
    std::vector<glm::vec4> weight4567;
};
Volume_weight calculate_weight(Model &gl_model, glm::ivec3 probe_res, glm::ivec3 volume_res, glm::vec3 scene_size);