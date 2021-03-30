#version 430 core
layout (location = 0) in vec3 aPos;

out vec3 CubePos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    CubePos = aPos;
    mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  