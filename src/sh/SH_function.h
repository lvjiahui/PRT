#pragma once

#include <glm/vec3.hpp> // glm::vec3

using vec3 = glm::vec3;
// d directX coord
float SH00(const vec3 d) {
  return 0.282095;
}

float SH1n1(const vec3 d) {
  return 0.488603 * d.y;
}

float SH10(const vec3 d) {
  return 0.488603 * d.z;
}

float SH1p1(const vec3 d) {
  return 0.488603 * d.x;
}

float SH2n2(const vec3 d) {
  return 1.092548 * d.x * d.y;
}

float SH2n1(const vec3 d) {
  return 1.092548 * d.y * d.z;
}

float SH20(const vec3 d) {
  return 0.315392 * (3.0 * d.z * d.z - 1);
}

float SH2p1(const vec3 d) {
  return 1.092548 * d.x * d.z;
}

float SH2p2(const vec3 d) {
  return 0.546274 * (d.x * d.x - d.y * d.y);
}

class SH9{
public:
  SH9(){
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[8] = 0;
  }
  SH9(vec3 d){ //directX coord
    data[0] = SH00(d) ;
    data[1] = SH1n1(d);
    data[2] = SH10(d) ;
    data[3] = SH1p1(d);
    data[4] = SH2n2(d);
    data[5] = SH2n1(d);
    data[6] = SH20(d) ;
    data[7] = SH2p1(d);
    data[8] = SH2p2(d);
  }
  SH9& operator+=(const SH9& other){
    data[0] += other.data[0];
    data[1] += other.data[1];
    data[2] += other.data[2];
    data[3] += other.data[3];
    data[4] += other.data[4];
    data[5] += other.data[5];
    data[6] += other.data[6];
    data[7] += other.data[7];
    data[8] += other.data[8];
    return *this;
  }
  SH9 operator*(GLfloat m){
    SH9 res{};
    res.data[0] = m * data[0];
    res.data[1] = m * data[1];
    res.data[2] = m * data[2];
    res.data[3] = m * data[3];
    res.data[4] = m * data[4];
    res.data[5] = m * data[5];
    res.data[6] = m * data[6];
    res.data[7] = m * data[7];
    res.data[8] = m * data[8];
    return res;
  }
  GLfloat data[9];
};


vec3 cubeCoordToWorld(int x, int y, int face)
{
    float u = (x + 0.5) / 64.0;
    float v = (y + 0.5) / 64.0;
    u = u  * 2.0 - 1.0; // -1..1
    v = v  * 2.0 - 1.0; // -1..1
    switch(face)
    {
        case 0: return vec3(1.0, -v, -u); // posx
        case 1: return vec3(-1.0, -v, u); //negx
        case 2: return vec3(u, 1.0, v); // posy
        case 3: return vec3(u, -1.0, -v); //negy
        case 4: return vec3(u, -v, 1.0); // posz
        case 5: return vec3(-u, -v, -1.0); // negz
    }
    return vec3(0.0);
}