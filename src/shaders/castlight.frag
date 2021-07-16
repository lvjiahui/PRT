#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
in vec4 FragPosLightSpace;

#include common/light.glsl
uniform CastLight light;
uniform PointLight ambient;
uniform ParalLight sky;

#include common/SH.glsl
uniform bool sh;
const float sh_shift = 0;

#include colored_wall.glsl
#include common/paral_shadow.glsl


void main()
{
    vec3 rad = vec3(0, 0, 0);
    vec3 N = normalize(Normal);

    float shadow = ShadowCalculation(FragPosLightSpace, sky.direction, N);
    rad += (1-shadow) * Eval_ParalLight(sky, N);
    rad += Eval_CastLight(light, WorldPos, N);
    rad += Eval_PointLight(ambient, WorldPos, N);
    if(sh)
        rad += SH_Irad(vec4(N,1), WorldPos + sh_shift*N) / PI;

    vec3 albedo = get_albedo(WorldPos);

    vec3 color = albedo * rad;
    // tonemap
    color = color / (color + vec3(1.0));
    // gamma
    color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1);

}