#version 430 core
out vec4 FragColor;

in vec3 CubeTexPos;

uniform float lod = 0.0;
uniform samplerCube environment;
uniform bool tonemap;
uniform bool gamma;
uniform bool white_bk;

void main()
{   
    // vec3 color = texture(environment, CubeTexPos).rgb;
    vec3 color = textureLod(environment, CubeTexPos, lod).rgb;
    if(tonemap)
        color = color / (color + vec3(1.0));
    if(gamma)
        color = pow(color, vec3(1.0/2.2)); 
    if(white_bk)
        color = vec3(1.0);
    FragColor = vec4(color, 1.0);
}