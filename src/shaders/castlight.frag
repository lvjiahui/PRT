#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  

#include common/light.glsl
uniform CastLight light;
uniform PointLight ambient;

#include common/SH.glsl
#include colored_wall.glsl


void main()
{
    vec3 rad = vec3(0, 0, 0);
    vec3 N = normalize(Normal);

    rad += Eval_CastLight(light, WorldPos, N);
    rad += Eval_PointLight(ambient, WorldPos, N);
    rad += SH_Irad(vec4(N,1), WorldPos) / PI;

    vec3 albedo = get_albedo(WorldPos);

    vec3 color = albedo * rad;
    // tonemap
    color = color / (color + vec3(1.0));
    // gamma
    color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1);

}