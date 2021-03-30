#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
  
uniform vec3 objectColor;
uniform vec3 cameraPos;
uniform bool tonemap;
uniform bool gamma;

uniform samplerCube skybox;
uniform samplerCube irradiance;

void main()
{
    // vec3 I = normalize(WorldPos - cameraPos);
    // vec3 R = reflect(I, normalize(Normal));
    vec3 color = texture(skybox, normalize(Normal)).rgb;
    if(tonemap)
        color = color / (color + vec3(1.0));
    if(gamma)
        color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1.0);
    // FragColor = vec4(Normal, 1.0);
} 