#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
  
uniform vec3 objectColor;
uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{
    vec3 I = normalize(WorldPos - cameraPos);
    vec3 R = reflect(I, normalize(Normal));
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
    // FragColor = vec4(Normal, 1.0);
} 