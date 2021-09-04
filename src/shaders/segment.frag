#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  

float rand(float n){
    return fract(sin(n) * 43758.5453123);
}

int argmax(vec3 v){
    if (v.x >= v.y && v.x >= v.z) return 0;
    if (v.y >= v.x && v.y >= v.z) return 1;
    if (v.z >= v.x && v.z >= v.y) return 2;
}

/*
	enum class Dir{
		POSITIVE_X,
		NEGATIVE_X,
		POSITIVE_Y,
		NEGATIVE_Y,
		POSITIVE_Z,
		NEGATIVE_Z,
	};
*/
int principal_dir(vec3 v){
    int dir = argmax(abs(v));
    dir = 2*dir + int(v[dir] < 0);
    return dir;
}

ivec3 spatial_segment(vec3 pos){
    pos = floor(1.f * pos);
    return ivec3(pos);
}

vec3 segment_color(vec3 pos, vec3 N){
    ivec3 section = spatial_segment(pos);
    section *= ivec3(10000, 100, 1);
    int dir = principal_dir(N);
    float r = rand(section.x + section.y + section.z + dir);
    float g = rand(section.x + section.y + section.z + dir + 1);
    float b = rand(section.x + section.y + section.z + dir + 2);
    return 0.8 * vec3(r,g,b) + 0.2;
}


void main()
{
    vec3 N = normalize(Normal);
    FragColor = vec4(segment_color(WorldPos, N), 1.0);
} 