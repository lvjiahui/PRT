#version 430 core
out vec4 FragColor;

in vec3 CubePos;

uniform samplerCube skybox;
uniform bool tonemap;
uniform bool gamma;

void main()
{   
    vec3 color = texture(skybox, CubePos).rgb;
    if(tonemap)
        color = color / (color + vec3(1.0));
    if(gamma)
        color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1.0);
}