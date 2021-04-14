#version 430 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 WorldPos;  
in vec3 Sh_color;  
  
// material parameters
uniform vec3 albedo;
uniform bool metal;
uniform float roughness;
uniform vec3 F0;

uniform mat4 envRotate;
uniform vec3 cameraPos;
uniform bool tonemap;
uniform bool gamma;
uniform bool diffuse;
uniform bool specular;
uniform bool sh;

uniform sampler2D brdfLUT;
uniform samplerCube prefilterMap;
uniform samplerCube environment;
uniform samplerCube irradiance;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - WorldPos);
    vec3 R = reflect(-V, N); 
    vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);        
    vec3 color = vec3(0.0);
    vec3 specular_color = vec3(0.0);
    vec3 diffuse_color = vec3(0.0);
    if (specular){
        // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL  specular part.
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, vec3(envRotate*vec4(R, 1.0)),  roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        specular_color = prefilteredColor * (F * brdf.x + brdf.y); //specular
    }
    if(diffuse && !metal){
        if(sh)
            diffuse_color = albedo * Sh_color; //sh contains 1/PI
        else
            diffuse_color = albedo/PI * texture(irradiance, vec3(envRotate*vec4(N, 1.0))).rgb; //lambertian
    }
    if(specular && diffuse)
        color = specular_color + (1-F)*diffuse_color;
    else if(specular)
        color = specular_color;
    else if(diffuse)
        color = diffuse_color;

    color = max(color, vec3(0));
    if(tonemap)
        color = color / (color + vec3(1.0));
    if(gamma)
        color = pow(color, vec3(1.0/2.2)); 
    FragColor = vec4(color, 1.0);
    // FragColor = vec4(Normal, 1.0);
} 