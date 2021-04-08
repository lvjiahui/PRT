#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in float Sh_coeff[9];

out vec3 WorldPos;
out vec3 Normal;
out vec3 Sh_color;
uniform vec3 env_sh[9];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Sh_color = vec3(0.0);
    for(int i = 0; i < 9; i++)
        Sh_color += Sh_coeff[i] * env_sh[i];
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}