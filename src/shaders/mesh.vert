#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 WorldPos;
out vec3 Normal;
out vec4 FragPosLightSpace; //shadow map pos

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * normalize(aNormal);
    FragPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0);

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}